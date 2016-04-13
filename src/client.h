#ifndef TWITTER_H_ABFF6A12
#define TWITTER_H_ABFF6A12

#include "codes.h"
#include "tweet.h"
#include "auth.h"
#include <list>
#include <curl_easy.h>
#include <curl_header.h>

namespace OAuth {
  class Consumer;
  class Token;
  class Client;
};

namespace twitter {
  
  class client {
    public:
      client(const auth& _auth);
      ~client();
      
      response updateStatus(std::string msg, tweet& result, std::list<long> media_ids = {});
      response uploadMedia(std::string media_type, const char* data, long data_length, long& media_id);
      
    private:
      OAuth::Consumer* _oauth_consumer;
      OAuth::Token* _oauth_token;
      OAuth::Client* _oauth_client;
      
      bool performGet(std::string url, long& response_code, json& result);
      bool performPost(std::string url, std::string dataStr, long& response_code, json& result);
      bool performMultiPost(std::string url, const curl_httppost* fields, long& response_code, json& result);
      response codeForError(int httpcode, json errors) const;
  };
  
};

#endif /* end of include guard: TWITTER_H_ABFF6A12 */
