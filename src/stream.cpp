#include "stream.h"
#include <liboauthcpp/liboauthcpp.h>
#include <curl_easy.h>
#include <curl_header.h>
#include "util.h"
#include "notification.h"
#include "client.h"

namespace twitter {

  stream::stream(
    const client& tclient,
    notify_callback callback,
    bool with_followings,
    bool receive_all_replies,
    std::list<std::string> track,
    std::list<bounding_box> locations) :
      _client(tclient),
      _notify(callback),
      _thread(&stream::run, this, generateUrl(with_followings, receive_all_replies, track, locations))
  {
  }
  
  stream::~stream()
  {
    if (_thread.joinable())
    {
      _stop = true;
      _thread.join();
    }
  }
  
  std::string stream::generateUrl(
    bool with_followings,
    bool receive_all_replies,
    std::list<std::string> track,
    std::list<bounding_box> locations)
  {
    std::list<std::string> arguments;
    
    if (receive_all_replies)
    {
      arguments.push_back("replies=all");
    }
    
    if (!with_followings)
    {
      arguments.push_back("with=user");
    }
    
    if (!track.empty())
    {
      std::ostringstream trackstr;
      trackstr << "track=";
      
      for (auto it = std::begin(track); it != std::end(track); it++)
      {
        if (it != std::begin(track))
        {
          trackstr << ",";
        }
        
        trackstr << OAuth::HttpEncodeQueryValue(*it);
      }
      
      arguments.push_back(trackstr.str());
    }
    
    if (!locations.empty())
    {
      std::ostringstream localstr;
      localstr << "locations=";
      
      for (auto it = std::begin(locations); it != std::end(locations); it++)
      {
        if (it != std::begin(locations))
        {
          localstr << ",";
        }
        
        localstr << (double)it->getSouthWestLongitude() << ",";
        localstr << (double)it->getSouthWestLatitude() << ",";
        localstr << (double)it->getNorthEastLongitude() << ",";
        localstr << (double)it->getNorthEastLatitude();
      }
      
      arguments.push_back(localstr.str());
    }
    
    std::ostringstream urlstr;
    urlstr << "https://userstream.twitter.com/1.1/user.json";
    
    if (!arguments.empty())
    {
      urlstr << "?";
      urlstr << implode(std::begin(arguments), std::end(arguments), "&");
    }
    
    return urlstr.str();
  }
  
  void stream::run(std::string url)
  {
    curl::curl_ios<stream> ios(this, [] (void* contents, size_t size, size_t nmemb, void* userp) {
      return static_cast<stream*>(userp)->write(static_cast<char*>(contents), size, nmemb);
    });
    
    curl::curl_easy conn(ios);
    curl::curl_header headers;
    std::string oauth_header;
    
    try
    {
      oauth_header = _client._oauth_client->getFormattedHttpHeader(OAuth::Http::Get, url, "");
      
      if (!oauth_header.empty())
      {
        headers.add(oauth_header);
      }
    } catch (const OAuth::ParseError& error)
    {
      std::cout << "Error generating OAuth header:" << std::endl;
      std::cout << error.what() << std::endl;
      std::cout << "This is likely due to a malformed URL." << std::endl;
  
      assert(false);
    }
    
    try
    {
      conn.add<CURLOPT_HEADERFUNCTION>(nullptr);
      conn.add<CURLOPT_HEADERDATA>(nullptr);
      conn.add<CURLOPT_XFERINFOFUNCTION>([] (void* cdata, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
        return static_cast<stream*>(cdata)->progress();
      });
      conn.add<CURLOPT_XFERINFODATA>(this);
      conn.add<CURLOPT_NOPROGRESS>(0);
      //conn.add<CURLOPT_VERBOSE>(1);
      //conn.add<CURLOPT_DEBUGFUNCTION>(my_trace);
      conn.add<CURLOPT_URL>(url.c_str());
      conn.add<CURLOPT_HTTPHEADER>(headers.get());
    } catch (const curl::curl_exception& error)
    {
      error.print_traceback();
      
      assert(false);
    }
    
    _backoff_type = backoff::none;
    _backoff_amount = std::chrono::milliseconds(0);
    for (;;)
    {
      bool failure = false;
      try
      {
        conn.perform();
      } catch (const curl::curl_easy_exception& error)
      {
        failure = true;
        if ((error.get_code() == CURLE_ABORTED_BY_CALLBACK) && _stop)
        {
          break;
        } else {
          if (_backoff_type == backoff::none)
          {
            _established = false;
            _backoff_type = backoff::network;
            _backoff_amount = std::chrono::milliseconds(0);
          }
        }
      }
      
      if (!failure)
      {
        long response_code = conn.get_info<CURLINFO_RESPONSE_CODE>().get();
        if (response_code == 420)
        {
          if (_backoff_type == backoff::none)
          {
            _established = false;
            _backoff_type = backoff::rate_limit;
            _backoff_amount = std::chrono::minutes(1);
          }
        } else if (response_code != 200)
        {
          if (_backoff_type == backoff::none)
          {
            _established = false;
            _backoff_type = backoff::http;
            _backoff_amount = std::chrono::seconds(5);
          }
        } else {
          if (_backoff_type == backoff::none)
          {
            _established = false;
            _backoff_type = backoff::network;
            _backoff_amount = std::chrono::milliseconds(0);
          }
        }
      }
      
      std::this_thread::sleep_for(_backoff_amount);

      switch (_backoff_type)
      {
        case backoff::network:
        {
          if (_backoff_amount < std::chrono::seconds(16))
          {
            _backoff_amount += std::chrono::milliseconds(250);
          }
        
          break;
        }
      
        case backoff::http:
        {
          if (_backoff_amount < std::chrono::seconds(320))
          {
            _backoff_amount *= 2;
          }
        
          break;
        }
      
        case backoff::rate_limit:
        {
          _backoff_amount *= 2;
        
          break;
        }
        
        case backoff::none:
        { 
          break;
        }
      }
    }
  }
  
  size_t stream::write(char* ptr, size_t size, size_t nmemb)
  {
    for (size_t i = 0; i < size*nmemb; i++)
    {
      if (ptr[i] == '\r')
      {
        i++; // Skip the \n
        
        if (!_buffer.empty())
        {
          notification n(_client, _buffer);
          if (n.getType() == notification::type::friends)
          {
            _established = true;
            _backoff_type = backoff::none;
            _backoff_amount = std::chrono::milliseconds(0);
          }
          
          _notify(n);
                    
          _buffer = "";
        }
      } else {
        _buffer.push_back(ptr[i]);
      }
    }
    
    time(&_last_write);
    
    return size*nmemb;
  }
  
  int stream::progress()
  {
    if (_stop)
    {
      return 1;
    }
    
    if (_established)
    {
      if (difftime(time(NULL), _last_write) >= 90)
      {
        return 1;
      }
    }
    
    return 0;
  }
  
}
