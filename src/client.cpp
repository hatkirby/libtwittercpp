#include "client.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <liboauthcpp/liboauthcpp.h>
#include "util.h"

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
  
  response client::updateStatus(std::string msg, tweet& result, std::list<long> media_ids)
  {
    std::stringstream datastrstream;
    datastrstream << "status=" << OAuth::PercentEncode(msg);
    
    if (!media_ids.empty())
    {
      datastrstream << "&media_ids=";
      datastrstream << twitter::implode(std::begin(media_ids), std::end(media_ids), ",");
    }
    
    std::string datastr = datastrstream.str();
    std::string url = "https://api.twitter.com/1.1/statuses/update.json";
    
    long response_code;
    json response_data;
    if (!performPost(url, datastr, response_code, response_data))
    {
      return response::curl_error;
    }
    
    if (response_code == 200)
    {
      result = tweet(response_data);
      return response::ok;
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
    curl::curl_pair<CURLformoption, std::string> bytes_cont(CURLFORM_COPYCONTENTS, std::to_string(data_length));
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
    json response_data;
    if (!performMultiPost("https://upload.twitter.com/1.1/media/upload.json", form.get(), response_code, response_data))
    {
      return response::curl_error;
    }
    
    if (response_code / 100 != 2)
    {
      return codeForError(response_code, response_data);
    }
    
    media_id = response_data["media_id"].get<long>();

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
    curl::curl_pair<CURLformoption, std::string> media_id_cont(CURLFORM_COPYCONTENTS, std::to_string(media_id));
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
    
    if (response_data.find("processing_info") != response_data.end())
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
        
        if (response_data["processing_info"]["state"] == "succeeded")
        {
          break;
        }
        
        int ttw = response_data["processing_info"]["check_after_secs"].get<int>();
        sleep(ttw);
      }
    }
    
    return response::ok;
  }
  
  bool client::performGet(std::string url, long& response_code, json& result)
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
    if (output.str().empty())
    {
      result = json();
    } else {
      result = json::parse(output.str());
    }
    
    return true;
  }
  
  bool client::performPost(std::string url, std::string datastr, long& response_code, json& result)
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
    if (output.str().empty())
    {
      result = json();
    } else {
      result = json::parse(output.str());
    }
    
    return true;
  }
  
  bool client::performMultiPost(std::string url, const curl_httppost* fields, long& response_code, json& result)
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
    
    if (output.str().empty())
    {
      result = json();
    } else {
      result = json::parse(output.str());
    }
    
    return true;
  }
  
  response client::codeForError(int response_code, json response_data) const
  {
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
  
};
