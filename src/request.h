#ifndef REQUEST_H_9D3C30E2
#define REQUEST_H_9D3C30E2

#include <string>
#include <sstream>
#include <curl_easy.h>
#include <curl_header.h>
#include "auth.h"

namespace twitter {

  class request
  {
  public:

    explicit request(std::string url);

    std::string perform();

  private:

    std::ostringstream output_;
    curl::curl_ios<std::ostringstream> ios_;

  protected:

    curl::curl_easy conn_;
  };

  class get : public request
  {
  public:

    get(
      const auth& tauth,
      std::string url);

  private:

    curl::curl_header headers_;
  };

  class post : public request
  {
  public:

    post(
      const auth& tauth,
      std::string url,
      std::string datastr);

  private:

    curl::curl_header headers_;
  };

  class multipost : public request
  {
  public:

    multipost(
      const auth& tauth,
      std::string url,
      const curl_httppost* fields);

  private:

    curl::curl_header headers_;
  };

}

#endif /* end of include guard: REQUEST_H_9D3C30E2 */
