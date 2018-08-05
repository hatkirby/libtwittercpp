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
#include "timeline.h"

namespace twitter {

  class client {
  public:

    client(const auth& arg);

    tweet updateStatus(std::string msg, std::list<long> media_ids = {}) const;
    long uploadMedia(std::string media_type, const char* data, long data_length) const;

    tweet replyToTweet(std::string msg, tweet_id in_response_to, std::list<long> media_ids = {}) const;

    std::set<user_id> getFriends(user_id id) const;
    std::set<user_id> getFriends(const user& u) const;
    std::set<user_id> getFriends() const;

    std::set<user_id> getFollowers(user_id id) const;
    std::set<user_id> getFollowers(const user& u) const;
    std::set<user_id> getFollowers() const;

    void follow(user_id toFollow) const;
    void follow(const user& toFollow) const;

    void unfollow(user_id toUnfollow) const;
    void unfollow(const user& toUnfollow) const;

    const user& getUser() const;

    const configuration& getConfiguration() const;

    timeline& getHomeTimeline()
    {
      return homeTimeline_;
    }

    timeline& getMentionsTimeline()
    {
      return mentionsTimeline_;
    }

  private:

    const auth& auth_;

    user currentUser_;

    mutable std::unique_ptr<configuration> _configuration;
    mutable time_t _last_configuration_update;

    timeline homeTimeline_ {
      auth_,
      "https://api.twitter.com/1.1/statuses/home_timeline.json"};

    timeline mentionsTimeline_ {
      auth_,
      "https://api.twitter.com/1.1/statuses/mentions_timeline.json"};
  };

};

#endif /* end of include guard: TWITTER_H_ABFF6A12 */
