#include "client.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <liboauthcpp/liboauthcpp.h>
#include <curl_easy.h>
#include <curl_header.h>
#include "util.h"
#include <json.hpp>

using nlohmann::json;

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
  
  int client_stream_progress_callback_wrapper(void* cdata, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
  {
    return static_cast<client::stream*>(cdata)->progress();
  }
  
  size_t client_stream_write_callback_wrapper(void* ptr, size_t size, size_t nmemb, void* cdata)
  {
    return static_cast<client::stream*>(cdata)->write(static_cast<char*>(ptr), size, nmemb);
  }
  
  client::client(const auth& _arg)
  {
    _oauth_consumer = new OAuth::Consumer(_arg.getConsumerKey(), _arg.getConsumerSecret());
    _oauth_token = new OAuth::Token(_arg.getAccessKey(), _arg.getAccessSecret());
    _oauth_client = new OAuth::Client(_oauth_consumer, _oauth_token);
    
    std::string url = "https://api.twitter.com/1.1/account/verify_credentials.json";
    long response_code;
    std::string response_data;
    if (performGet(url, response_code, response_data) && (response_code == 200))
    {
      try {
        _current_user = user(response_data);
      } catch (std::invalid_argument e)
      {
        // Ignore
      }
    }
  }
  
  client::~client()
  {
    delete _oauth_client;
    delete _oauth_token;
    delete _oauth_consumer;
  }
  
  response client::updateStatus(std::string msg, tweet& result, tweet in_response_to, std::list<long> media_ids)
  {
    std::stringstream datastrstream;
    datastrstream << "status=" << OAuth::PercentEncode(msg);
    
    if (in_response_to)
    {
      datastrstream << "&in_reply_to_status_id=";
      datastrstream << in_response_to.getID();
    }
    
    if (!media_ids.empty())
    {
      datastrstream << "&media_ids=";
      datastrstream << twitter::implode(std::begin(media_ids), std::end(media_ids), ",");
    }
    
    std::string datastr = datastrstream.str();
    std::string url = "https://api.twitter.com/1.1/statuses/update.json";
    
    long response_code;
    std::string response_data;
    if (!performPost(url, datastr, response_code, response_data))
    {
      return response::curl_error;
    }
    
    if (response_code == 200)
    {
      try {
        result = tweet(response_data);
        return response::ok;
      } catch (std::invalid_argument e)
      {
        return response::invalid_response;
      }
    } else {
      return codeForError(response_code, response_data);
    }
  }
  
  response client::uploadMedia(std::string media_type, const char* data, long data_length, long& media_id)
  {
    curl::curl_form form;
    
    curl::curl_pair<CURLformoption, std::string> command_name(CURLFORM_COPYNAME, "command");
    curl::curl_pair<CURLformoption, std::string> command_cont(CURLFORM_COPYCONTENTS, "INIT");
    curl::curl_pair<CURLformoption, std::string> bytes_name(CURLFORM_COPYNAME, "total_bytes");
    std::string str_data_length = std::to_string(data_length);
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
    
    long response_code;
    std::string response_data;
    if (!performMultiPost("https://upload.twitter.com/1.1/media/upload.json", form.get(), response_code, response_data))
    {
      return response::curl_error;
    }
    
    if (response_code / 100 != 2)
    {
      return codeForError(response_code, response_data);
    }
    
    json response_json;
    try {
      response_json = json::parse(response_data);
    } catch (std::invalid_argument e)
    {
      return response::invalid_response;
    }
    
    media_id = response_json["media_id"].get<long>();

    curl_httppost* append_form_post = nullptr;
    curl_httppost* append_form_last = nullptr;
    curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "command", CURLFORM_COPYCONTENTS, "APPEND", CURLFORM_END);
    curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "media_id", CURLFORM_COPYCONTENTS, std::to_string(media_id).c_str(), CURLFORM_END);
    curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "media", CURLFORM_BUFFER, "media", CURLFORM_BUFFERPTR, data, CURLFORM_BUFFERLENGTH, data_length, CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END);
    curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "segment_index", CURLFORM_COPYCONTENTS, std::to_string(0).c_str(), CURLFORM_END);
    if (!performMultiPost("https://upload.twitter.com/1.1/media/upload.json", append_form_post, response_code, response_data))
    {
      return response::curl_error;
    }
    
    curl_formfree(append_form_post);
    
    if (response_code / 100 != 2)
    {
      return codeForError(response_code, response_data);
    }
    
    curl::curl_form finalize_form;
    curl::curl_pair<CURLformoption, std::string> command3_name(CURLFORM_COPYNAME, "command");
    curl::curl_pair<CURLformoption, std::string> command3_cont(CURLFORM_COPYCONTENTS, "FINALIZE");
    curl::curl_pair<CURLformoption, std::string> media_id_name(CURLFORM_COPYNAME, "media_id");
    std::string str_media_id = std::to_string(media_id);
    curl::curl_pair<CURLformoption, std::string> media_id_cont(CURLFORM_COPYCONTENTS, str_media_id);
    finalize_form.add(command3_name, command3_cont);
    finalize_form.add(media_id_name, media_id_cont);
    
    if (!performMultiPost("https://upload.twitter.com/1.1/media/upload.json", finalize_form.get(), response_code, response_data))
    {
      return response::curl_error;
    }
    
    if (response_code / 100 != 2)
    {
      return codeForError(response_code, response_data);
    }
    
    try {
      response_json = json::parse(response_data);
    } catch (std::invalid_argument e)
    {
      return response::invalid_response;
    }
    
    if (response_json.find("processing_info") != response_json.end())
    {
      std::stringstream datastr;
      datastr << "https://upload.twitter.com/1.1/media/upload.json?command=STATUS&media_id=" << media_id;
      
      for (;;)
      {
        if (!performGet(datastr.str(), response_code, response_data))
        {
          return response::curl_error;
        }
        
        if (response_code / 100 != 2)
        {
          return codeForError(response_code, response_data);
        }
        
        try {
          response_json = json::parse(response_data);
        } catch (std::invalid_argument e)
        {
          return response::invalid_response;
        }
        
        if (response_json["processing_info"]["state"] == "succeeded")
        {
          break;
        }
        
        int ttw = response_json["processing_info"]["check_after_secs"].get<int>();
        std::this_thread::sleep_for(std::chrono::seconds(ttw));
      }
    }
    
    return response::ok;
  }
  
  response client::follow(user_id toFollow)
  {
    std::stringstream datastrstream;
    datastrstream << "follow=true&user_id=";
    datastrstream << toFollow;
    
    std::string datastr = datastrstream.str();
    std::string url = "https://api.twitter.com/1.1/friendships/create.json";
    
    long response_code;
    std::string response_data;
    if (!performPost(url, datastr, response_code, response_data))
    {
      return response::curl_error;
    }
    
    if (response_code == 200)
    {
      return response::ok;
    } else {
      return codeForError(response_code, response_data);
    }
  }
  
  response client::follow(user toFollow)
  {
    return follow(toFollow.getID());
  }
  
  response client::unfollow(user_id toUnfollow)
  {
    std::stringstream datastrstream;
    datastrstream << "user_id=";
    datastrstream << toUnfollow;
    
    std::string datastr = datastrstream.str();
    std::string url = "https://api.twitter.com/1.1/friendships/destroy.json";
    
    long response_code;
    std::string response_data;
    if (!performPost(url, datastr, response_code, response_data))
    {
      return response::curl_error;
    }
    
    if (response_code == 200)
    {
      return response::ok;
    } else {
      return codeForError(response_code, response_data);
    }
  }
  
  response client::unfollow(user toUnfollow)
  {
    return unfollow(toUnfollow.getID());
  }
  
  response client::getUser(user& result)
  {
    if (!_current_user)
    {
      std::string url = "https://api.twitter.com/1.1/account/verify_credentials.json";
      long response_code;
      std::string response_data;
      if (performGet(url, response_code, response_data) && (response_code == 200))
      {
        try {
          _current_user = user(response_data);
        } catch (std::invalid_argument e)
        {
          return response::invalid_response;
        }
      }
    }
    
    result = _current_user;
    return response::ok;
  }
  
  configuration client::getConfiguration()
  {
    if (!_configuration || (difftime(time(NULL), _last_configuration_update) > 60*60*24))
    {
      long response_code;
      std::string response_data;
      if (performGet("https://api.twitter.com/1.1/help/configuration.json", response_code, response_data))
      {
        _configuration = configuration(response_data);
        _last_configuration_update = time(NULL);
      }
    }
    
    return _configuration;
  }
  
  response client::getFriends(std::set<user_id>& _ret)
  {
    if (!_current_user)
    {
      return response::unknown_error;
    }
    
    long long cursor = -1;
    std::set<user_id> result;
    
    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/friends/ids.json?user_id=";
      urlstream << _current_user.getID();
      urlstream << "&cursor=";
      urlstream << cursor;
  
      std::string url = urlstream.str();
  
      long response_code;
      std::string response_data;
      if (!performGet(url, response_code, response_data))
      {
        return response::curl_error;
      }
  
      if (response_code == 200)
      {
        json rjs;
        try {
          rjs = json::parse(response_data);
        } catch (std::invalid_argument e)
        {
          return response::invalid_response;
        }
        
        cursor = rjs.at("next_cursor");
        result.insert(std::begin(rjs.at("ids")), std::end(rjs.at("ids")));
      } else {
        return codeForError(response_code, response_data);
      }
    }
    
    _ret = result;
    
    return response::ok;
  }
  
  response client::getFollowers(std::set<user_id>& _ret)
  {
    if (!_current_user)
    {
      return response::unknown_error;
    }
    
    long long cursor = -1;
    std::set<user_id> result;
    
    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/followers/ids.json?user_id=";
      urlstream << _current_user.getID();
      urlstream << "&cursor=";
      urlstream << cursor;
  
      std::string url = urlstream.str();
  
      long response_code;
      std::string response_data;
      if (!performGet(url, response_code, response_data))
      {
        return response::curl_error;
      }
  
      if (response_code == 200)
      {
        json rjs;
        try {
          rjs = json::parse(response_data);
        } catch (std::invalid_argument e)
        {
          return response::invalid_response;
        }
        
        cursor = rjs.at("next_cursor");
        result.insert(std::begin(rjs.at("ids")), std::end(rjs.at("ids")));
      } else {
        return codeForError(response_code, response_data);
      }
    }
    
    _ret = result;
    
    return response::ok;
  }
  
  void client::setUserStreamNotifyCallback(stream::notify_callback callback)
  {
    _user_stream.setNotifyCallback(callback);
  }
  
  void client::setUserStreamReceiveAllReplies(bool _arg)
  {
    _user_stream.setReceiveAllReplies(_arg);
  }
  
  void client::startUserStream()
  {
    _user_stream.start();
  }
  
  void client::stopUserStream()
  {
    _user_stream.stop();
  }
  
  std::string client::generateReplyPrefill(tweet _tweet) const
  {
    std::ostringstream output;
    output << "@" << _tweet.getAuthor().getScreenName() << " ";
    
    for (auto mention : _tweet.getMentions())
    {
      if ((mention.first != _tweet.getAuthor().getID()) && (mention.first != _current_user.getID()))
      {
        output << "@" << mention.second << " ";
      }
    }
    
    return output.str();
  }
  
  bool client::performGet(std::string url, long& response_code, std::string& result)
  {
    std::ostringstream output;
    curl::curl_ios<std::ostringstream> ios(output);
    curl::curl_easy conn(ios);
    
    curl::curl_header headers;
    std::string oauth_header = _oauth_client->getFormattedHttpHeader(OAuth::Http::Get, url, "");
    if (!oauth_header.empty())
    {
      headers.add(oauth_header);
    }
    
    try {
      //conn.add<CURLOPT_VERBOSE>(1);
      //conn.add<CURLOPT_DEBUGFUNCTION>(my_trace);
      conn.add<CURLOPT_URL>(url.c_str());
      conn.add<CURLOPT_HTTPHEADER>(headers.get());
      
      conn.perform();
    } catch (curl::curl_easy_exception error)
    {
      error.print_traceback();
      
      return false;
    }
    
    response_code = conn.get_info<CURLINFO_RESPONSE_CODE>().get();
    result = output.str();
    
    return true;
  }
  
  bool client::performPost(std::string url, std::string datastr, long& response_code, std::string& result)
  {
    std::ostringstream output;
    curl::curl_ios<std::ostringstream> ios(output);
    curl::curl_easy conn(ios);
    
    curl::curl_header headers;
    std::string oauth_header = _oauth_client->getFormattedHttpHeader(OAuth::Http::Post, url, datastr);
    if (!oauth_header.empty())
    {
      headers.add(oauth_header);
    }
    
    try {
      //conn.add<CURLOPT_VERBOSE>(1);
      //conn.add<CURLOPT_DEBUGFUNCTION>(my_trace);
      conn.add<CURLOPT_URL>(url.c_str());
      conn.add<CURLOPT_COPYPOSTFIELDS>(datastr.c_str());
      conn.add<CURLOPT_HTTPHEADER>(headers.get());
      
      conn.perform();
    } catch (curl::curl_easy_exception error)
    {
      error.print_traceback();
      
      return false;
    }
    
    response_code = conn.get_info<CURLINFO_RESPONSE_CODE>().get();
    result = output.str();
    
    return true;
  }
  
  bool client::performMultiPost(std::string url, const curl_httppost* fields, long& response_code, std::string& result)
  {
    std::ostringstream output;
    curl::curl_ios<std::ostringstream> ios(output);
    curl::curl_easy conn(ios);
    
    curl::curl_header headers;
    std::string oauth_header = _oauth_client->getFormattedHttpHeader(OAuth::Http::Post, url, "");
    if (!oauth_header.empty())
    {
      headers.add(oauth_header);
    }
    
    try {
      //conn.add<CURLOPT_VERBOSE>(1);
      //conn.add<CURLOPT_DEBUGFUNCTION>(my_trace);
      conn.add<CURLOPT_HTTPHEADER>(headers.get());
      conn.add<CURLOPT_URL>(url.c_str());
      conn.add<CURLOPT_HTTPPOST>(fields);
      
      conn.perform();
    } catch (curl::curl_easy_exception error)
    {
      error.print_traceback();
      
      return false;
    }
    
    response_code = conn.get_info<CURLINFO_RESPONSE_CODE>().get();
    result = output.str();
    
    return true;
  }
  
  response client::codeForError(int response_code, std::string response_data) const
  {
    json response_json;
    try {
      response_json = json::parse(response_data);
    } catch (std::invalid_argument e)
    {
      return response::invalid_response;
    }
    
    std::set<int> error_codes;
    if (response_json.find("errors") != response_json.end())
    {
      std::transform(std::begin(response_json["errors"]), std::end(response_json["errors"]), std::inserter(error_codes, std::begin(error_codes)), [] (const json& error) {
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
    } else if (error_codes.count(44) == 1)
    {
      return response::invalid_media;
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
  
  client::stream::stream(client& _client) : _client(_client)
  {
    
  }
  
  bool client::stream::isRunning() const
  {
    return _thread.joinable();
  }
  
  void client::stream::setNotifyCallback(notify_callback _n)
  {
    std::lock_guard<std::mutex> _running_lock(_running_mutex);
    
    if (!_thread.joinable())
    {
      _notify = _n;
    }
  }
  
  void client::stream::setReceiveAllReplies(bool _arg)
  {
    std::lock_guard<std::mutex> _running_lock(_running_mutex);
    
    if (!_thread.joinable())
    {
      _receive_all_replies = _arg;
    }
  }
    
  void client::stream::start()
  {
    std::lock_guard<std::mutex> _running_lock(_running_mutex);
    
    if (!_thread.joinable())
    {
      _thread = std::thread(&stream::run, this);
    }
  }
  
  void client::stream::stop()
  {
    std::lock_guard<std::mutex> _running_lock(_running_mutex);
    
    if (_thread.joinable())
    {
      _stop = true;
      _thread.join();
      _stop = false;
    }
  }
  
  void client::stream::run()
  {
    curl::curl_easy conn;
    std::ostringstream urlstr;
    urlstr << "https://userstream.twitter.com/1.1/user.json";
    
    if (_receive_all_replies)
    {
      urlstr << "?replies=all";
    }
    
    std::string url = urlstr.str();
    curl::curl_header headers;
    std::string oauth_header = _client._oauth_client->getFormattedHttpHeader(OAuth::Http::Get, url, "");
    if (!oauth_header.empty())
    {
      headers.add(oauth_header);
    }
    
    conn.add<CURLOPT_WRITEFUNCTION>(client_stream_write_callback_wrapper);
    conn.add<CURLOPT_WRITEDATA>(this);
    conn.add<CURLOPT_HEADERFUNCTION>(nullptr);
    conn.add<CURLOPT_HEADERDATA>(nullptr);
    conn.add<CURLOPT_XFERINFOFUNCTION>(client_stream_progress_callback_wrapper);
    conn.add<CURLOPT_XFERINFODATA>(this);
    conn.add<CURLOPT_NOPROGRESS>(0);
    //conn.add<CURLOPT_VERBOSE>(1);
    //conn.add<CURLOPT_DEBUGFUNCTION>(my_trace);
    conn.add<CURLOPT_URL>(url.c_str());
    conn.add<CURLOPT_HTTPHEADER>(headers.get());
    
    _backoff_type = backoff::none;
    _backoff_amount = std::chrono::milliseconds(0);
    for (;;)
    {
      bool failure = false;
      try {
        conn.perform();
      } catch (curl::curl_easy_exception error)
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
      }
    }
  }
  
  size_t client::stream::write(char* ptr, size_t size, size_t nmemb)
  {
    for (size_t i = 0; i < size*nmemb; i++)
    {
      if (ptr[i] == '\r')
      {
        i++; // Skip the \n
        
        if (!_buffer.empty())
        {
          notification n(_buffer, _client._current_user);
          if (n.getType() == notification::type::friends)
          {
            _established = true;
            _backoff_type = backoff::none;
            _backoff_amount = std::chrono::milliseconds(0);
          }
          
          if (_notify)
          {
            _notify(n);
          }
                    
          _buffer = "";
        }
      } else {
        _buffer.push_back(ptr[i]);
      }
    }
    
    {
      std::lock_guard<std::mutex> _stall_lock(_stall_mutex);
      time(&_last_write);
    }
    
    return size*nmemb;
  }
  
  int client::stream::progress()
  {
    if (_stop)
    {
      return 1;
    }
    
    if (_established)
    {
      std::lock_guard<std::mutex> _stall_lock(_stall_mutex);
      if (difftime(time(NULL), _last_write) >= 90)
      {
        return 1;
      }
    }
    
    return 0;
  }
  
};
