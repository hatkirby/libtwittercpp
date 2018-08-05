#ifndef AUTH_H_48EF85FD
#define AUTH_H_48EF85FD

#include <string>
#include "../vendor/liboauthcpp/include/liboauthcpp/liboauthcpp.h"

namespace twitter {

  class auth {
  public:

    auth(
      std::string consumerKey,
      std::string consumerSecret,
      std::string accessKey,
      std::string accessSecret) :
        consumer_(std::move(consumerKey), std::move(consumerSecret)),
        token_(std::move(accessKey), std::move(accessSecret)),
        client_(&consumer_, &token_)
    {
    }

    const OAuth::Client& getClient() const
    {
      return client_;
    }

  private:

    OAuth::Consumer consumer_;
    OAuth::Token token_;
    OAuth::Client client_;
  };

};

#endif /* end of include guard: AUTH_H_48EF85FD */
