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

    notification() {}
    notification(const user& currentUser, std::string data);

    notification(const notification& other);
    notification(notification&& other);
    notification& operator=(notification other);
    ~notification();

    friend void swap(notification& first, notification& second);

    const tweet& getTweet() const;
    tweet& getTweet()
    {
      return const_cast<tweet&>(static_cast<const notification&>(*this).getTweet());
    }

    const user& getUser() const;
    user& getUser()
    {
      return const_cast<user&>(static_cast<const notification&>(*this).getUser());
    }

    const list& getList() const;
    list& getList()
    {
      return const_cast<list&>(static_cast<const notification&>(*this).getList());
    }

    tweet_id getTweetID() const;
    void setTweetID(tweet_id _arg);

    user_id getUserID() const;
    void setUserID(user_id _arg);

    const std::vector<std::string>& getCountries() const;
    std::vector<std::string>& getCountries()
    {
      return const_cast<std::vector<std::string>&>(static_cast<const notification&>(*this).getCountries());
    }

    disconnect_code getDisconnectCode() const;
    void setDisconnectCode(disconnect_code _arg);

    const std::set<user_id>& getFriends() const;
    std::set<user_id>& getFriends()
    {
      return const_cast<std::set<user_id>&>(static_cast<const notification&>(*this).getFriends());
    }

    const direct_message& getDirectMessage() const;
    direct_message& getDirectMessage()
    {
      return const_cast<direct_message&>(static_cast<const notification&>(*this).getDirectMessage());
    }

    int getLimit() const;
    void setLimit(int _arg);

    const std::string& getWarning() const;
    std::string& getWarning()
    {
      return const_cast<std::string&>(static_cast<const notification&>(*this).getWarning());
    }

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
    type _type = type::invalid;
  };

};

#endif /* end of include guard: NOTIFICATION_H_69AEF4CC */
