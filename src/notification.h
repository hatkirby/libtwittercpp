#ifndef NOTIFICATION_H_69AEF4CC
#define NOTIFICATION_H_69AEF4CC

#include <string>
#include <vector>
#include <set>
#include "tweet.h"
#include "user.h"
#include "list.h"
#include "direct_message.h"

namespace twitter {
  
  class client;
  
  enum class disconnect_code
  {
    shutdown,
    duplicate,
    stall,
    normal,
    token_revoked,
    admin_logout,
    limit,
    exception,
    broker,
    load,
    unknown
  };
  
  class notification {
  public:
    enum class type {
      // Tweet object
      tweet,
      
      // User object
      update_user,
      block,
      unblock,
      follow,
      followed,
      unfollow,
      
      // User and tweet
      favorite,
      favorited,
      unfavorite,
      unfavorited,
      quoted,
      
      // List
      list_created,
      list_destroyed,
      list_updated,
      
      // User and list
      list_add,
      list_added,
      list_remove,
      list_removed,
      list_subscribe,
      list_subscribed,
      list_unsubscribe,
      list_unsubscribed,
      
      // Warning
      stall,
      follow_limit,
      unknown_warning,
      
      // User ID and tweet ID
      deletion,
      scrub_location,
      
      // Special
      limit,
      withhold_status,
      withhold_user,
      disconnect,
      friends,
      direct,
      
      // Nothing
      unknown,
      invalid
    };
    
    type getType() const;
    
    notification(const client& tclient, std::string data);
    notification(notification&& other);
    notification& operator=(notification&& other);
    ~notification();
    
    notification(const notification& other) = delete;
    notification& operator=(const notification& other) = delete;
    
    const tweet& getTweet() const;
    const user& getUser() const;
    const list& getList() const;
    tweet_id getTweetID() const;
    user_id getUserID() const;
    const std::vector<std::string>& getCountries() const;
    disconnect_code getDisconnectCode() const;
    const std::set<user_id>& getFriends() const;
    const direct_message& getDirectMessage() const;
    int getLimit() const;
    const std::string& getWarning() const;
    
  private:
    union {
      tweet _tweet;
      user _user;
      list _list;
      struct {
        user _user;
        tweet _tweet;
      } _user_and_tweet;
      struct {
        user _user;
        list _list;
      } _user_and_list;
      std::string _warning;
      struct {
        user_id _user_id;
        tweet_id _tweet_id;
      } _user_id_and_tweet_id;
      int _limit;
      struct {
        user_id _user_id;
        tweet_id _tweet_id;
        std::vector<std::string> _countries;
      } _withhold_status;
      struct {
        user_id _user_id;
        std::vector<std::string> _countries;
      } _withhold_user;
      disconnect_code _disconnect;
      std::set<user_id> _friends;
      direct_message _direct_message;
    };
    type _type;
  };
  
};

#endif /* end of include guard: NOTIFICATION_H_69AEF4CC */
