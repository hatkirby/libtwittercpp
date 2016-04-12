#ifndef TWITTER_H_AC7A7666
#define TWITTER_H_AC7A7666

namespace twitter {
  
  enum class response {
    ok,
    curl_error,
    bad_auth,
    limited,
    server_error,
    server_unavailable,
    server_overloaded,
    server_timeout,
    suspended,
    bad_token,
    duplicate_status,
    suspected_spam,
    write_restricted,
    bad_length,
    unknown_error
  };
  
  class tweet;
  
};

#include <json.hpp>

using nlohmann::json;

#include "auth.h"
#include "client.h"
#include "tweet.h"

#endif /* end of include guard: TWITTER_H_AC7A7666 */
