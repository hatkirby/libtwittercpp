#include "client.h"
#include <curl_easy.h>
#include <curl_header.h>
#include <sstream>
#include <set>
#include <algorithm>
#include <liboauthcpp/liboauthcpp.h>

namespace twitter {
  
  client::client(const auth& _arg)
  {
    _oauth_consumer = new OAuth::Consumer(_arg.getConsumerKey(), _arg.getConsumerSecret());
    _oauth_token = new OAuth::Token(_arg.getAccessKey(), _arg.getAccessSecret());
    _oauth_client = new OAuth::Client(_oauth_consumer, _oauth_token);
  }
  
  client::~client()
  {
    delete _oauth_client;
    delete _oauth_token;
    delete _oauth_consumer;
  }
  
  response client::updateStatus(std::string msg, tweet& result)
  {
    std::ostringstream output;
    curl::curl_ios<std::ostringstream> ios(output);
    curl::curl_easy conn(ios);
    
    std::stringstream datastrstream;
    datastrstream << "status=" << OAuth::PercentEncode(msg);
    
    std::string datastr = datastrstream.str();
    std::string url = "https://api.twitter.com/1.1/statuses/update.json";
    
    curl::curl_header headers;
    std::string oauth_header = _oauth_client->getFormattedHttpHeader(OAuth::Http::Post, url, datastr);
    if (!oauth_header.empty())
    {
      headers.add(oauth_header);
    }
    
    try {
      conn.add<CURLOPT_URL>(url.c_str());
      conn.add<CURLOPT_COPYPOSTFIELDS>(datastr.c_str());
      conn.add<CURLOPT_POST>(1);
      conn.add<CURLOPT_HTTPHEADER>(headers.get());
      
      conn.perform();
    } catch (curl::curl_easy_exception error)
    {
      error.print_traceback();
      
      return response::curl_error;
    }
    
    long response_code = conn.get_info<CURLINFO_RESPONSE_CODE>().get();
    json response_data = json::parse(output.str());
    if (response_code == 200)
    {
      result = tweet(response_data);
      return response::ok;
    }
    
    std::set<int> error_codes;
    if (response_data.find("errors") != response_data.end())
    {
      std::transform(std::begin(response_data["errors"]), std::end(response_data["errors"]), std::inserter(error_codes, std::begin(error_codes)), [] (const json& error) {
        return error["code"].get<int>();
      });
    }
    
    if (error_codes.count(32) == 1 || error_codes.count(135) == 1 || error_codes.count(215) == 1)
    {
      return response::bad_auth;
    } else if (error_codes.count(64) == 1)
    {
      return response::suspended;
    } else if (error_codes.count(88) == 1 || error_codes.count(185) == 1)
    {
      return response::limited;
    } else if (error_codes.count(89) == 1)
    {
      return response::bad_token;
    } else if (error_codes.count(130) == 1)
    {
      return response::server_overloaded;
    } else if (error_codes.count(131) == 1)
    {
      return response::server_error;
    } else if (error_codes.count(186) == 1)
    {
      return response::bad_length;
    } else if (error_codes.count(187) == 1)
    {
      return response::duplicate_status;
    } else if (error_codes.count(226) == 1)
    {
      return response::suspected_spam;
    } else if (error_codes.count(261) == 1)
    {
      return response::write_restricted;
    } else if (response_code == 429)
    {
      return response::limited;
    } else if (response_code == 500)
    {
      return response::server_error;
    } else if (response_code == 502)
    {
      return response::server_unavailable;
    } else if (response_code == 503)
    {
      return response::server_overloaded;
    } else if (response_code == 504)
    {
      return response::server_timeout;
    } else {
      return response::unknown_error;
    }
  }
  
};
