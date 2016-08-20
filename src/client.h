#ifndef TWITTER_H_ABFF6A12
#define TWITTER_H_ABFF6A12

#include <list>
#include <set>
#include <ctime>
#include <memory>
#include "codes.h"
#include "tweet.h"
#include "auth.h"
#include "configuration.h"
#include "util.h"

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
    
    tweet updateStatus(std::string msg, std::list<long> media_ids = {}) const;
    long uploadMedia(std::string media_type, const char* data, long data_length) const;
    
    tweet replyToTweet(std::string msg, tweet_id in_response_to, std::list<long> media_ids = {}) const;
    std::set<user_id> getFriends(user_id id) const;
    std::set<user_id> getFollowers(user_id id) const;
    void follow(user_id toFollow) const;
    void unfollow(user_id toUnfollow) const;
    
    const user& getUser() const;
    
    const configuration& getConfiguration() const;
    
  private:
    
    friend class stream;
    
    std::unique_ptr<OAuth::Consumer> _oauth_consumer;
    std::unique_ptr<OAuth::Token> _oauth_token;
    std::unique_ptr<OAuth::Client> _oauth_client;
    
    std::unique_ptr<user> _current_user;
    
    mutable std::unique_ptr<configuration> _configuration;
    mutable time_t _last_configuration_update;
  };
  
};

#endif /* end of include guard: TWITTER_H_ABFF6A12 */
