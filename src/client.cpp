#include "client.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <liboauthcpp/liboauthcpp.h>
#include <curl_easy.h>
#include <curl_header.h>
#include <json.hpp>
#include <thread>

// These are here for debugging curl stuff

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size)
{
  size_t i;
  size_t c;
  unsigned int width=80;
 
  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);
 
  for(i=0; i<size; i+= width) {
    fprintf(stream, "%4.4lx: ", (long)i);
 
    /* show hex to the left
    for(c = 0; c < width; c++) {
      if(i+c < size)
        fprintf(stream, "%02x ", ptr[i+c]);
      else
        fputs("   ", stream);
    }*/
 
    /* show data on the right */
    for(c = 0; (c < width) && (i+c < size); c++) {
      char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
      fputc(x, stream);
    }
 
    fputc('\n', stream); /* newline */
  }
}
 
static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  const char *text;
  (void)handle; /* prevent compiler warning */
 
  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }
 
  dump(text, stderr, (unsigned char *)data, size);
  return 0;
}

namespace twitter {
  
  class request
  {
  public:
    
    explicit request(std::string url) try
      : _ios(_output), _conn(_ios)
    {
      _conn.add<CURLOPT_URL>(url.c_str());
    } catch (const curl::curl_easy_exception& error)
    {
      error.print_traceback();
    
      assert(false);
    }
    
    std::string perform()
    {
      try
      {
        _conn.perform();
      } catch (const curl::curl_easy_exception& error)
      {
        std::throw_with_nested(connection_error());
      }
    
      int response_code = _conn.get_info<CURLINFO_RESPONSE_CODE>().get();
      std::string result = _output.str();
    
      if (response_code / 100 != 2)
      {
        nlohmann::json response_json;
      
        try
        {
          response_json = nlohmann::json::parse(result);
        } catch (const std::invalid_argument& e)
        {
          std::throw_with_nested(invalid_response(result));
        }
    
        for (nlohmann::json& error : response_json["errors"])
        {
          int error_code;
          std::string error_message;
      
          try
          {
            error_code = error["code"].get<int>();
            error_message = error["message"].get<std::string>();
          } catch (const std::domain_error& e)
          {
            std::throw_with_nested(invalid_response(result));
          }
      
          switch (error_code)
          {
          case 32:
          case 135:
          case 215:
            throw bad_auth(error_message);
        
          case 44:
            throw invalid_media(error_message);
        
          case 64:
            throw account_suspended(error_message);
        
          case 88:
            throw rate_limit_exceeded(error_message);
        
          case 89:
            throw bad_token(error_message);
        
          case 130:
            throw server_overloaded(error_message);
        
          case 131:
            throw server_error(error_message);
      
          case 185:
            throw update_limit_exceeded(error_message);
        
          case 186:
            throw bad_length(error_message);
        
          case 187:
            throw duplicate_status(error_message);
        
          case 226:
            throw suspected_spam(error_message);
        
          case 261:
            throw write_restricted(error_message);
          }
        }

        if (response_code == 429)
        {
          throw rate_limit_exceeded("HTTP 429 Too Many Requests");
        } else if (response_code == 500)
        {
          throw server_error("HTTP 500 Internal Server Error");
        } else if (response_code == 502)
        {
          throw server_unavailable("HTTP 502 Bad Gateway");
        } else if (response_code == 503)
        {
          throw server_overloaded("HTTP 503 Service Unavailable");
        } else if (response_code == 504)
        {
          throw server_timeout("HTTP 504 Gateway Timeout");
        }
      
        throw unknown_error(response_code, result);
      }
    
      return result;
    }
    
  private:
    
    std::ostringstream _output;
    curl::curl_ios<std::ostringstream> _ios;
    
  protected:
    
    curl::curl_easy _conn;
  };
  
  class get : public request
  {
  public:
    
    get(const OAuth::Client& oauth_client, std::string url) try
      : request(url)
    {
      std::string oauth_header = oauth_client.getFormattedHttpHeader(OAuth::Http::Get, url, "");
      if (!oauth_header.empty())
      {
        _headers.add(std::move(oauth_header));
      }
  
      _conn.add<CURLOPT_HTTPHEADER>(_headers.get());
    } catch (const OAuth::ParseError& error)
    {
      std::cout << "Error generating OAuth header:" << std::endl;
      std::cout << error.what() << std::endl;
      std::cout << "This is likely due to a malformed URL." << std::endl;
    
      assert(false);
    } catch (const curl::curl_easy_exception& error)
    {
      error.print_traceback();
  
      assert(false);
    }
    
  private:
    
    curl::curl_header _headers;
  };
  
  class post : public request
  {
  public:
    
    post(const OAuth::Client& oauth_client, std::string url, std::string datastr) try
      : request(url)
    {
      std::string oauth_header = oauth_client.getFormattedHttpHeader(OAuth::Http::Post, url, datastr);
      if (!oauth_header.empty())
      {
        _headers.add(std::move(oauth_header));
      }
  
      _conn.add<CURLOPT_HTTPHEADER>(_headers.get());
      _conn.add<CURLOPT_COPYPOSTFIELDS>(datastr.c_str());
    } catch (const OAuth::ParseError& error)
    {
      std::cout << "Error generating OAuth header:" << std::endl;
      std::cout << error.what() << std::endl;
      std::cout << "This is likely due to a malformed URL." << std::endl;
    
      assert(false);
    } catch (const curl::curl_easy_exception& error)
    {
      error.print_traceback();
  
      assert(false);
    }
    
  private:
    
    curl::curl_header _headers;
  };
  
  class multipost : public request
  {
  public:
    
    multipost(const OAuth::Client& oauth_client, std::string url, const curl_httppost* fields) try
      : request(url)
    {
      std::string oauth_header = oauth_client.getFormattedHttpHeader(OAuth::Http::Post, url, "");
      if (!oauth_header.empty())
      {
        _headers.add(std::move(oauth_header));
      }
  
      _conn.add<CURLOPT_HTTPHEADER>(_headers.get());
      _conn.add<CURLOPT_HTTPPOST>(fields);
    } catch (const OAuth::ParseError& error)
    {
      std::cout << "Error generating OAuth header:" << std::endl;
      std::cout << error.what() << std::endl;
      std::cout << "This is likely due to a malformed URL." << std::endl;
    
      assert(false);
    } catch (const curl::curl_easy_exception& error)
    {
      error.print_traceback();
  
      assert(false);
    }
    
  private:
    
    curl::curl_header _headers;
  };
    
  client::client(const auth& _arg)
  {
    _oauth_consumer =
      make_unique<OAuth::Consumer>(
        _arg.getConsumerKey(),
        _arg.getConsumerSecret());
          
    _oauth_token =
      make_unique<OAuth::Token>(
        _arg.getAccessKey(),
        _arg.getAccessSecret());
          
    _oauth_client =
      make_unique<OAuth::Client>(
        _oauth_consumer.get(),
        _oauth_token.get());
    
    _current_user =
      make_unique<user>(
        get(*_oauth_client,
          "https://api.twitter.com/1.1/account/verify_credentials.json")
        .perform());
  }
  
  client::~client() = default;
  
  tweet client::updateStatus(std::string msg, std::list<long> media_ids) const
  {
    std::stringstream datastrstream;
    datastrstream << "status=" << OAuth::PercentEncode(msg);
    
    if (!media_ids.empty())
    {
      datastrstream << "&media_ids=";
      datastrstream << twitter::implode(std::begin(media_ids), std::end(media_ids), ",");
    }
    
    return tweet(
      post(*_oauth_client,
        "https://api.twitter.com/1.1/statuses/update.json",
        datastrstream.str())
      .perform());
  }
  
  tweet client::replyToTweet(std::string msg, tweet_id in_response_to, std::list<long> media_ids) const
  {
    std::stringstream datastrstream;
    datastrstream << "status=" << OAuth::PercentEncode(msg);
    datastrstream << "&in_reply_to_status_id=";
    datastrstream << in_response_to;
    
    if (!media_ids.empty())
    {
      datastrstream << "&media_ids=";
      datastrstream << twitter::implode(std::begin(media_ids), std::end(media_ids), ",");
    }
    
    return tweet(
      post(*_oauth_client,
        "https://api.twitter.com/1.1/statuses/update.json",
        datastrstream.str())
      .perform());
  }
  
  tweet client::replyToTweet(std::string msg, const tweet& in_response_to, std::list<long> media_ids) const
  {
    return replyToTweet(msg, in_response_to.getID(), media_ids);
  }
  
  long client::uploadMedia(std::string media_type, const char* data, long data_length) const try
  {
    curl::curl_form form;
    std::string str_data_length = std::to_string(data_length);
    
    curl::curl_pair<CURLformoption, std::string> command_name(CURLFORM_COPYNAME, "command");
    curl::curl_pair<CURLformoption, std::string> command_cont(CURLFORM_COPYCONTENTS, "INIT");
    curl::curl_pair<CURLformoption, std::string> bytes_name(CURLFORM_COPYNAME, "total_bytes");
    curl::curl_pair<CURLformoption, std::string> bytes_cont(CURLFORM_COPYCONTENTS, str_data_length);
    curl::curl_pair<CURLformoption, std::string> type_name(CURLFORM_COPYNAME, "media_type");
    curl::curl_pair<CURLformoption, std::string> type_cont(CURLFORM_COPYCONTENTS, media_type);
    form.add(command_name, command_cont);
    form.add(bytes_name, bytes_cont);
    form.add(type_name, type_cont);

    if (media_type == "image/gif")
    {
      curl::curl_pair<CURLformoption, std::string> category_name(CURLFORM_COPYNAME, "media_category");
      curl::curl_pair<CURLformoption, std::string> category_cont(CURLFORM_COPYCONTENTS, "tweet_gif");
      form.add(category_name, category_cont);
    }
    
    std::string init_response =
      multipost(*_oauth_client,
        "https://upload.twitter.com/1.1/media/upload.json",
        form.get())
      .perform();

    long media_id;
    
    try
    {
      nlohmann::json response_json = nlohmann::json::parse(init_response);
      media_id = response_json["media_id"].get<long>();
    } catch (const std::invalid_argument& error)
    {
      std::throw_with_nested(invalid_response(init_response));
    } catch (const std::domain_error& error)
    {
      std::throw_with_nested(invalid_response(init_response));
    }

    // TODO: Currently have to use the C libcurl API to create this form because it uses a buffer and
    // libcurlcpp currently messes that up.
    curl_httppost* append_form_post = nullptr;
    curl_httppost* append_form_last = nullptr;
    if ( curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "command", CURLFORM_COPYCONTENTS, "APPEND", CURLFORM_END)
      || curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "media_id", CURLFORM_COPYCONTENTS, std::to_string(media_id).c_str(), CURLFORM_END)
      || curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "media", CURLFORM_BUFFER, "media", CURLFORM_BUFFERPTR, data, CURLFORM_BUFFERLENGTH, data_length, CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END)
      || curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "segment_index", CURLFORM_COPYCONTENTS, std::to_string(0).c_str(), CURLFORM_END))
    {
      assert(false);
    }
    
    multipost(*_oauth_client, "https://upload.twitter.com/1.1/media/upload.json", append_form_post).perform();
    
    curl_formfree(append_form_post);
    
    curl::curl_form finalize_form;
    std::string str_media_id = std::to_string(media_id);
    
    curl::curl_pair<CURLformoption, std::string> command3_name(CURLFORM_COPYNAME, "command");
    curl::curl_pair<CURLformoption, std::string> command3_cont(CURLFORM_COPYCONTENTS, "FINALIZE");
    curl::curl_pair<CURLformoption, std::string> media_id_name(CURLFORM_COPYNAME, "media_id");
    curl::curl_pair<CURLformoption, std::string> media_id_cont(CURLFORM_COPYCONTENTS, str_media_id);
    finalize_form.add(command3_name, command3_cont);
    finalize_form.add(media_id_name, media_id_cont);
    
    std::string finalize_response =
      multipost(*_oauth_client,
        "https://upload.twitter.com/1.1/media/upload.json",
        finalize_form.get())
      .perform();

    nlohmann::json finalize_json;
    
    try
    {
      finalize_json = nlohmann::json::parse(finalize_response);
    } catch (const std::invalid_argument& error)
    {
      std::throw_with_nested(invalid_response(finalize_response));
    }
    
    if (finalize_json.find("processing_info") != finalize_json.end())
    {
      std::stringstream datastr;
      datastr << "https://upload.twitter.com/1.1/media/upload.json?command=STATUS&media_id=" << media_id;
      
      for (;;)
      {
        std::string status_response = get(*_oauth_client, datastr.str()).perform();
        
        try
        {
          nlohmann::json status_json = nlohmann::json::parse(status_response);
          std::string state = status_json["processing_info"]["state"].get<std::string>();
          
          if (state == "succeeded")
          {
            break;
          }
          
          int ttw = status_json["processing_info"]["check_after_secs"].get<int>();
          std::this_thread::sleep_for(std::chrono::seconds(ttw));
        } catch (const std::invalid_argument& error)
        {
          std::throw_with_nested(invalid_response(status_response));
        } catch (const std::domain_error& error)
        {
          std::throw_with_nested(invalid_response(status_response));
        }
      }
    }
    
    return media_id;
  } catch (const curl::curl_exception& error)
  {
    error.print_traceback();
    
    assert(false);
  }
  
  std::set<user_id> client::getFriends(user_id id) const
  {
    long long cursor = -1;
    std::set<user_id> result;
    
    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/friends/ids.json?user_id=";
      urlstream << id;
      urlstream << "&cursor=";
      urlstream << cursor;
  
      std::string url = urlstream.str();
      std::string response_data = get(*_oauth_client, url).perform();
      
      try
      {
        nlohmann::json rjs = nlohmann::json::parse(response_data);
        
        cursor = rjs["next_cursor"].get<long long>();
        result.insert(std::begin(rjs["ids"]), std::end(rjs["ids"]));
      } catch (const std::invalid_argument& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      } catch (const std::domain_error& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      }
    }
    
    return result;
  }
  
  std::set<user_id> client::getFriends(const user& id) const
  {
    return getFriends(id.getID());
  }
  
  std::set<user_id> client::getFriends() const
  {
    return getFriends(getUser().getID());
  }
  
  std::set<user_id> client::getFollowers(user_id id) const
  {
    long long cursor = -1;
    std::set<user_id> result;
    
    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/followers/ids.json?user_id=";
      urlstream << id;
      urlstream << "&cursor=";
      urlstream << cursor;
  
      std::string url = urlstream.str();
      std::string response_data = get(*_oauth_client, url).perform();
      
      try
      {
        nlohmann::json rjs = nlohmann::json::parse(response_data);
        
        cursor = rjs["next_cursor"].get<long long>();
        result.insert(std::begin(rjs["ids"]), std::end(rjs["ids"]));
      } catch (const std::invalid_argument& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      } catch (const std::domain_error& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      }
    }
    
    return result;
  }
  
  std::set<user_id> client::getFollowers(const user& id) const
  {
    return getFollowers(id.getID());
  }
  
  std::set<user_id> client::getFollowers() const
  {
    return getFollowers(getUser().getID());
  }

  std::set<user_id> client::getBlocks() const
  {
    long long cursor = -1;
    std::set<user_id> result;

    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/blocks/ids.json?cursor=";
      urlstream << cursor;

      std::string url = urlstream.str();
      std::string response_data = get(*_oauth_client, url).perform();

      try
      {
        nlohmann::json rjs = nlohmann::json::parse(response_data);

        cursor = rjs["next_cursor"].get<long long>();
        result.insert(std::begin(rjs["ids"]), std::end(rjs["ids"]));
      } catch (const std::invalid_argument& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      } catch (const std::domain_error& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      }
    }

    return result;
  }

  void client::follow(user_id toFollow) const
  {
    std::stringstream datastrstream;
    datastrstream << "follow=true&user_id=";
    datastrstream << toFollow;
    
    post(*_oauth_client, "https://api.twitter.com/1.1/friendships/create.json", datastrstream.str()).perform();
  }
  
  void client::follow(const user& toFollow) const
  {
    return follow(toFollow.getID());
  }
  
  void client::unfollow(user_id toUnfollow) const
  {
    std::stringstream datastrstream;
    datastrstream << "user_id=";
    datastrstream << toUnfollow;
    
    post(*_oauth_client, "https://api.twitter.com/1.1/friendships/destroy.json", datastrstream.str()).perform();
  }
  
  void client::unfollow(const user& toUnfollow) const
  {
    return unfollow(toUnfollow.getID());
  }

  const user& client::getUser() const
  {
    return *_current_user;
  }
  
  const configuration& client::getConfiguration() const
  {
    if (!_configuration || (difftime(time(NULL), _last_configuration_update) > 60*60*24))
    {
      _configuration =
        make_unique<configuration>(
          get(*_oauth_client,
            "https://api.twitter.com/1.1/help/configuration.json")
          .perform());

      _last_configuration_update = time(NULL);
    }
    
    return *_configuration;
  }
  
};
