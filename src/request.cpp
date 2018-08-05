#include "request.h"
#include <json.hpp>
#include "codes.h"

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

  request::request(
    std::string url) try :
      ios_(output_),
      conn_(ios_)
  {
    conn_.add<CURLOPT_URL>(url.c_str());
  } catch (const curl::curl_easy_exception& error)
  {
    std::throw_with_nested(connection_error());
  }

  std::string request::perform()
  {
    try
    {
      conn_.perform();
    } catch (const curl::curl_easy_exception& error)
    {
      std::throw_with_nested(connection_error());
    }

    int response_code = conn_.get_info<CURLINFO_RESPONSE_CODE>().get();
    std::string result = output_.str();

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

  get::get(
    const auth& tauth,
    std::string url) try :
      request(url)
  {
    std::string oauthHeader =
      tauth.getClient().getFormattedHttpHeader(OAuth::Http::Get, url, "");

    if (!oauthHeader.empty())
    {
      headers_.add(std::move(oauthHeader));
    }

    conn_.add<CURLOPT_HTTPHEADER>(headers_.get());
  } catch (const OAuth::ParseError& error)
  {
    std::throw_with_nested(connection_error());
  } catch (const curl::curl_easy_exception& error)
  {
    std::throw_with_nested(connection_error());
  }

  post::post(
    const auth& tauth,
    std::string url,
    std::string datastr) try :
      request(url)
  {
    std::string oauthHeader =
      tauth.getClient().getFormattedHttpHeader(OAuth::Http::Post, url, datastr);

    if (!oauthHeader.empty())
    {
      headers_.add(std::move(oauthHeader));
    }

    conn_.add<CURLOPT_HTTPHEADER>(headers_.get());
    conn_.add<CURLOPT_COPYPOSTFIELDS>(datastr.c_str());
  } catch (const OAuth::ParseError& error)
  {
    std::throw_with_nested(connection_error());
  } catch (const curl::curl_easy_exception& error)
  {
    std::throw_with_nested(connection_error());
  }

  multipost::multipost(
    const auth& tauth,
    std::string url,
    const curl_httppost* fields) try :
      request(url)
  {
    std::string oauthHeader =
      tauth.getClient().getFormattedHttpHeader(OAuth::Http::Post, url, "");

    if (!oauthHeader.empty())
    {
      headers_.add(std::move(oauthHeader));
    }

    conn_.add<CURLOPT_HTTPHEADER>(headers_.get());
    conn_.add<CURLOPT_HTTPPOST>(fields);
  } catch (const OAuth::ParseError& error)
  {
    std::throw_with_nested(connection_error());
  } catch (const curl::curl_easy_exception& error)
  {
    std::throw_with_nested(connection_error());
  }

}
