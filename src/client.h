#ifndef TWITTER_H_ABFF6A12
#define TWITTER_H_ABFF6A12

#include "twitter.h"

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
      
      response updateStatus(std::string msg, tweet& result);
      
    private:
      OAuth::Consumer* _oauth_consumer;
      OAuth::Token* _oauth_token;
      OAuth::Client* _oauth_client;
  };
  
};

#endif /* end of include guard: TWITTER_H_ABFF6A12 */
