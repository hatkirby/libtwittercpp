#include "notification.h"
#include <cassert>
#include <new>
#include <json.hpp>
#include "codes.h"
#include "client.h"

namespace twitter {
  
  notification::type notification::getType() const
  {
    return _type;
  }
  
  notification::notification(const client& tclient, std::string data)
  {
    const user& current_user = tclient.getUser();
    
    nlohmann::json json;
    
    try
    {
      json = nlohmann::json::parse(data);
    } catch (const std::invalid_argument& error)
    {
      std::throw_with_nested(invalid_response(data));
    }
    
    try
    {
      if (!json["event"].is_null())
      {
        std::string event = json["event"];
        user source(json["source"].dump());
        user target(json["target"].dump());
      
        if (event == "user_update")
        {
          _type = type::update_user;
        
          new(&_user) user(source);
        } else if (event == "block")
        {
          _type = type::block;
        
          new(&_user) user(target);
        } else if (event == "unblock")
        {
          _type = type::unblock;
        
          new(&_user) user(target);
        } else if (event == "favorite")
        {
          new(&_user_and_tweet._tweet) tweet(json["target_object"].dump());
        
          if (current_user == source)
          {
            _type = type::favorite;
          
            new(&_user_and_tweet._user) user(target);
          } else {
            _type = type::favorited;
          
            new(&_user_and_tweet._user) user(source);
          }
        } else if (event == "unfavorite")
        {
          new(&_user_and_tweet._tweet) tweet(json["target_object"].dump());
        
          if (current_user == source)
          {
            _type = type::unfavorite;
          
            new(&_user_and_tweet._user) user(target);
          } else {
            _type = type::unfavorited;
          
            new(&_user_and_tweet._user) user(source);
          }
        } else if (event == "follow")
        {
          if (current_user == source)
          {
            _type = type::follow;
          
            new(&_user) user(target);
          } else {
            _type = type::followed;
          
            new(&_user) user(source);
          }
        } else if (event == "unfollow")
        {
          _type = type::unfollow;
        
          new(&_user) user(target);
        } else if (event == "list_created")
        {
          _type = type::list_created;
        
          new(&_list) list(json["target_object"].dump());
        } else if (event == "list_destroyed")
        {
          _type = type::list_destroyed;
        
          new(&_list) list(json["target_object"].dump());
        } else if (event == "list_updated")
        {
          _type = type::list_updated;
        
          new(&_list) list(json["target_object"].dump());
        } else if (event == "list_member_added")
        {
          new(&_user_and_list._list) list(json["target_object"].dump());
        
          if (current_user == source)
          {
            _type = type::list_add;
          
            new(&_user_and_list._user) user(target);
          } else {
            _type = type::list_added;
          
            new(&_user_and_list._user) user(source);
          }
        } else if (event == "list_member_removed")
        {
          new(&_user_and_list._list) list(json["target_object"].dump());
        
          if (current_user == source)
          {
            _type = type::list_remove;
          
            new(&_user_and_list._user) user(target);
          } else {
            _type = type::list_removed;
          
            new(&_user_and_list._user) user(source);
          }
        } else if (event == "list_member_subscribe")
        {
          new(&_user_and_list._list) list(json["target_object"].dump());
        
          if (current_user == source)
          {
            _type = type::list_subscribe;
          
            new(&_user_and_list._user) user(target);
          } else {
            _type = type::list_subscribed;
          
            new(&_user_and_list._user) user(source);
          }
        } else if (event == "list_member_unsubscribe")
        {
          new(&_user_and_list._list) list(json["target_object"].dump());
        
          if (current_user == source)
          {
            _type = type::list_unsubscribe;
          
            new(&_user_and_list._user) user(target);
          } else {
            _type = type::list_unsubscribed;
          
            new(&_user_and_list._user) user(source);
          }
        } else if (event == "quoted_tweet")
        {
          _type = type::quoted;
        
          new(&_user_and_tweet._user) user(source);
          new(&_user_and_tweet._tweet) tweet(json["target_object"].dump());
        } else {
          _type = type::unknown;
        }
      } else if (!json["warning"].is_null())
      {
        new(&_warning) std::string(json["warning"]["message"].get<std::string>());
      
        auto warning_code = json["warning"]["code"].get<std::string>();
        if (warning_code == "FALLING_BEHIND")
        {
          _type = type::stall;
        } else if (warning_code == "FOLLOWS_OVER_LIMIT")
        {
          _type = type::follow_limit;
        } else {
          _type = type::unknown_warning;
        }
      } else if (!json["delete"].is_null())
      {
        _type = type::deletion;
      
        _user_id_and_tweet_id._tweet_id = json["delete"]["status"]["id"].get<tweet_id>();
        _user_id_and_tweet_id._user_id = json["delete"]["status"]["user_id"].get<user_id>();
      } else if (!json["scrub_geo"].is_null())
      {
        _type = type::scrub_location;
      
        _user_id_and_tweet_id._tweet_id = json["scrub_geo"]["up_to_status_id"].get<tweet_id>();
        _user_id_and_tweet_id._user_id = json["scrub_geo"]["user_id"].get<user_id>();
      } else if (!json["limit"].is_null())
      {
        _type = type::limit;
      
        _limit = json["limit"]["track"].get<int>();
      } else if (!json["status_withheld"].is_null())
      {
        _type = type::withhold_status;
      
        _withhold_status._user_id = json["status_withheld"]["user_id"].get<user_id>();
        _withhold_status._tweet_id = json["status_withheld"]["id"].get<tweet_id>();
      
        new(&_withhold_status._countries) std::vector<std::string>();
        for (auto s : json["status_withheld"]["withheld_in_countries"])
        {
          _withhold_status._countries.push_back(s);
        }
      } else if (!json["user_withheld"].is_null())
      {
        _type = type::withhold_user;
      
        _withhold_user._user_id = json["user_withheld"]["id"].get<user_id>();
      
        new(&_withhold_user._countries) std::vector<std::string>();
        for (auto s : json["user_withheld"]["withheld_in_countries"])
        {
          _withhold_user._countries.push_back(s);
        }
      } else if (!json["disconnect"].is_null())
      {
        _type = type::disconnect;
      
        switch (json["disconnect"]["code"].get<int>())
        {
          case 1: _disconnect = disconnect_code::shutdown; break;
          case 2: _disconnect = disconnect_code::duplicate; break;
          case 4: _disconnect = disconnect_code::stall; break;
          case 5: _disconnect = disconnect_code::normal; break;
          case 6: _disconnect = disconnect_code::token_revoked; break;
          case 7: _disconnect = disconnect_code::admin_logout; break;
          case 9: _disconnect = disconnect_code::limit; break;
          case 10: _disconnect = disconnect_code::exception; break;
          case 11: _disconnect = disconnect_code::broker; break;
          case 12: _disconnect = disconnect_code::load; break;
          default: _disconnect = disconnect_code::unknown;
        }
      } else if (!json["friends"].is_null())
      {
        _type = type::friends;
      
        new(&_friends) std::set<user_id>(std::begin(json["friends"]), std::end(json["friends"]));
      } else if (!json["direct_message"].is_null())
      {
        _type = type::direct;
      
        new(&_direct_message) direct_message(json["direct_message"].dump());
      } else {
        _type = type::tweet;
      
        new(&_tweet) tweet(data);
      }
    } catch (const std::domain_error& error)
    {
      std::throw_with_nested(invalid_response(data));
    }
  }
  
  notification::notification(const notification& other)
  {
    _type = other._type;
    
    switch (_type)
    {
      case type::tweet:
      {
        new(&_tweet) tweet(other._tweet);
      
        break;
      }
    
      case type::update_user:
      case type::block:
      case type::unblock:
      case type::follow:
      case type::followed:
      case type::unfollow:
      {
        new(&_user) user(other._user);
      
        break;
      }
    
      case type::favorite:
      case type::favorited:
      case type::unfavorite:
      case type::unfavorited:
      case type::quoted:
      {
        new(&_user_and_tweet._user) user(other._user_and_tweet._user);
        new(&_user_and_tweet._tweet) tweet(other._user_and_tweet._tweet);
      
        break;
      }
    
      case type::list_created:
      case type::list_destroyed:
      case type::list_updated:
      {
        new(&_list) list(other._list);
      
        break;
      }
    
      case type::list_add:
      case type::list_added:
      case type::list_remove:
      case type::list_removed:
      case type::list_subscribe:
      case type::list_subscribed:
      case type::list_unsubscribe:
      case type::list_unsubscribed:
      {
        new(&_user_and_list._user) user(other._user_and_list._user);
        new(&_user_and_list._list) list(other._user_and_list._list);
      
        break;
      }
    
      case type::stall:
      case type::follow_limit:
      case type::unknown_warning:
      {
        new(&_warning) std::string(other._warning);
      
        break;
      }
    
      case type::deletion:
      case type::scrub_location:
      {
        _user_id_and_tweet_id._user_id = other._user_id_and_tweet_id._user_id;
        _user_id_and_tweet_id._tweet_id = other._user_id_and_tweet_id._tweet_id;
      
        break;
      }
    
      case type::limit:
      {
        _limit = other._limit;
      
        break;
      }
    
      case type::withhold_status:
      {
        _withhold_status._user_id = other._withhold_status._user_id;
        _withhold_status._tweet_id = other._withhold_status._tweet_id;
        new(&_withhold_status._countries) std::vector<std::string>(other._withhold_status._countries);
      
        break;
      }
    
      case type::withhold_user:
      {
        _withhold_user._user_id = other._withhold_user._user_id;
        new(&_withhold_user._countries) std::vector<std::string>(other._withhold_user._countries);
      
        break;
      }
    
      case type::disconnect:
      {
        _disconnect = other._disconnect;
      
        break;
      }
    
      case type::friends:
      {
        new(&_friends) std::set<user_id>(other._friends);
      
        break;
      }
    
      case type::direct:
      {
        new(&_direct_message) direct_message(other._direct_message);
      
        break;
      }
      
      case type::unknown:
      case type::invalid:
      {
        break;
      }
    }
  }
  
  notification::notification(notification&& other) : notification()
  {
    swap(*this, other);
  }
  
  notification& notification::operator=(notification other)
  {
    swap(*this, other);
    
    return *this;
  }
  
  notification::~notification()
  {
    switch (_type)
    {
      case type::tweet:
      {
        _tweet.~tweet();
        
        break;
      }
      
      case type::update_user:
      case type::block:
      case type::unblock:
      case type::follow:
      case type::followed:
      case type::unfollow:
      {
        _user.~user();
        
        break;
      }
      
      case type::favorite:
      case type::favorited:
      case type::unfavorite:
      case type::unfavorited:
      case type::quoted:
      {
        _user_and_tweet._user.~user();
        _user_and_tweet._tweet.~tweet();
        
        break;
      }
      
      case type::list_created:
      case type::list_destroyed:
      case type::list_updated:
      {
        _list.~list();
        
        break;
      }
      
      case type::list_add:
      case type::list_added:
      case type::list_remove:
      case type::list_removed:
      case type::list_subscribe:
      case type::list_subscribed:
      case type::list_unsubscribe:
      case type::list_unsubscribed:
      {
        _user_and_list._user.~user();
        _user_and_list._list.~list();
        
        break;
      }
      
      case type::stall:
      case type::follow_limit:
      case type::unknown_warning:
      {
        using string_type = std::string;
        _warning.~string_type();
        
        break;
      }
      
      case type::withhold_status:
      {
        using list_type = std::vector<std::string>;
        _withhold_status._countries.~list_type();
                
        break;
      }
      
      case type::withhold_user:
      {
        using list_type = std::vector<std::string>;
        _withhold_user._countries.~list_type();
        
        break;
      }
            
      case type::friends:
      {
        using list_type = std::set<user_id>;
        _friends.~list_type();
        
        break;
      }
      
      case type::direct:
      {
        _direct_message.~direct_message();
        
        break;
      }
      
      case type::deletion:
      case type::scrub_location:
      case type::limit:
      case type::disconnect:
      case type::unknown:
      case type::invalid:
      {
        break;
      }
    }
  }
  
  void swap(notification& first, notification& second)
  {
    using type = notification::type;
    
    type tempType = first._type;
    tweet tempTweet;
    user tempUser;
    list tempList;
    std::string tempWarning;
    user_id tempUserId;
    tweet_id tempTweetId;
    int tempLimit;
    std::vector<std::string> tempCountries;
    disconnect_code tempDisconnectCode;
    std::set<user_id> tempFriends;
    direct_message tempDirectMessage;
    
    switch (first._type)
    {
      case type::tweet:
      {
        tempTweet = std::move(first._tweet);
        
        break;
      }
      
      case type::update_user:
      case type::block:
      case type::unblock:
      case type::follow:
      case type::followed:
      case type::unfollow:
      {
        tempUser = std::move(first._user);
      
        break;
      }
    
      case type::favorite:
      case type::favorited:
      case type::unfavorite:
      case type::unfavorited:
      case type::quoted:
      {
        tempTweet = std::move(first._user_and_tweet._tweet);
        tempUser = std::move(first._user_and_tweet._user);
      
        break;
      }
    
      case type::list_created:
      case type::list_destroyed:
      case type::list_updated:
      {
        tempList = std::move(first._list);
      
        break;
      }
    
      case type::list_add:
      case type::list_added:
      case type::list_remove:
      case type::list_removed:
      case type::list_subscribe:
      case type::list_subscribed:
      case type::list_unsubscribe:
      case type::list_unsubscribed:
      {
        tempList = std::move(first._user_and_list._list);
        tempUser = std::move(first._user_and_list._user);
      
        break;
      }
    
      case type::stall:
      case type::follow_limit:
      case type::unknown_warning:
      {
        tempWarning = std::move(first._warning);
      
        break;
      }
    
      case type::deletion:
      case type::scrub_location:
      {
        tempUserId = first._user_id_and_tweet_id._user_id;
        tempTweetId = first._user_id_and_tweet_id._tweet_id;
      
        break;
      }
    
      case type::limit:
      {
        tempLimit = first._limit;
      
        break;
      }
    
      case type::withhold_status:
      {
        tempUserId = first._withhold_status._user_id;
        tempTweetId = first._withhold_status._tweet_id;
        tempCountries = std::move(first._withhold_status._countries);
      
        break;
      }
    
      case type::withhold_user:
      {
        tempUserId = first._withhold_user._user_id;
        tempCountries = std::move(first._withhold_user._countries);
      
        break;
      }
    
      case type::disconnect:
      {
        tempDisconnectCode = first._disconnect;
      
        break;
      }
    
      case type::friends:
      {
        tempFriends = std::move(first._friends);
      
        break;
      }
    
      case type::direct:
      {
        tempDirectMessage = std::move(first._direct_message);
      
        break;
      }
    
      case type::invalid:
      case type::unknown:
      {
        break;
      }
    }
    
    first.~notification();
    
    first._type = second._type;
    
    // Okay now you need to initialize the first with the data from the second
    // And then destruct the second and initialize it with the data stored in temp
    // This is hell
    
    switch (second._type)
    {
      case type::tweet:
      {
        new(&first._tweet) tweet(std::move(second._tweet));
        
        break;
      }
      
      case type::update_user:
      case type::block:
      case type::unblock:
      case type::follow:
      case type::followed:
      case type::unfollow:
      {
        new(&first._user) user(std::move(second._user));
        
        break;
      }
      
      case type::favorite:
      case type::favorited:
      case type::unfavorite:
      case type::unfavorited:
      case type::quoted:
      {
        new(&first._user_and_tweet._user) user(std::move(second._user_and_tweet._user));
        new(&first._user_and_tweet._tweet) tweet(std::move(second._user_and_tweet._tweet));
        
        break;
      }
      
      case type::list_created:
      case type::list_destroyed:
      case type::list_updated:
      {
        new(&first._list) list(std::move(second._list));
        
        break;
      }
      
      case type::list_add:
      case type::list_added:
      case type::list_remove:
      case type::list_removed:
      case type::list_subscribe:
      case type::list_subscribed:
      case type::list_unsubscribe:
      case type::list_unsubscribed:
      {
        new(&first._user_and_list._user) user(std::move(second._user_and_list._user));
        new(&first._user_and_list._list) list(std::move(second._user_and_list._list));
        
        break;
      }
      
      case type::stall:
      case type::follow_limit:
      case type::unknown_warning:
      {
        new(&first._warning) std::string(std::move(second._warning));
        
        break;
      }
      
      case type::deletion:
      case type::scrub_location:
      {
        first._user_id_and_tweet_id._user_id = second._user_id_and_tweet_id._user_id;
        first._user_id_and_tweet_id._tweet_id = second._user_id_and_tweet_id._tweet_id;
        
        break;
      }
      
      case type::limit:
      {
        first._limit = second._limit;
        
        break;
      }
      
      case type::withhold_status:
      {
        first._withhold_status._user_id = second._withhold_status._user_id;
        first._withhold_status._tweet_id = second._withhold_status._tweet_id;
        new(&first._withhold_status._countries) std::vector<std::string>(std::move(second._withhold_status._countries));
        
        break;
      }
      
      case type::withhold_user:
      {
        first._withhold_user._user_id = second._withhold_user._user_id;
        new(&first._withhold_user._countries) std::vector<std::string>(std::move(second._withhold_user._countries));
        
        break;
      }
      
      case type::disconnect:
      {
        first._disconnect = second._disconnect;
        
        break;
      }
      
      case type::friends:
      {
        new(&first._friends) std::set<user_id>(std::move(second._friends));
        
        break;
      }
      
      case type::direct:
      {
        new(&first._direct_message) direct_message(std::move(second._direct_message));
        
        break;
      }
      
      case type::invalid:
      case type::unknown:
      {
        break;
      }
    }
    
    // Now destruct the second and initialize it with data from the first
    second.~notification();
    
    second._type = tempType;
    
    switch (tempType)
    {
      case type::tweet:
      {
        new(&second._tweet) tweet(std::move(tempTweet));
      
        break;
      }
    
      case type::update_user:
      case type::block:
      case type::unblock:
      case type::follow:
      case type::followed:
      case type::unfollow:
      {
        new(&second._user) user(std::move(tempUser));
      
        break;
      }
    
      case type::favorite:
      case type::favorited:
      case type::unfavorite:
      case type::unfavorited:
      case type::quoted:
      {
        new(&second._user_and_tweet._user) user(std::move(tempUser));
        new(&second._user_and_tweet._tweet) tweet(std::move(tempTweet));
      
        break;
      }
    
      case type::list_created:
      case type::list_destroyed:
      case type::list_updated:
      {
        new(&second._list) list(std::move(tempList));
      
        break;
      }
    
      case type::list_add:
      case type::list_added:
      case type::list_remove:
      case type::list_removed:
      case type::list_subscribe:
      case type::list_subscribed:
      case type::list_unsubscribe:
      case type::list_unsubscribed:
      {
        new(&second._user_and_list._user) user(std::move(tempUser));
        new(&second._user_and_list._list) list(std::move(tempList));
      
        break;
      }
    
      case type::stall:
      case type::follow_limit:
      case type::unknown_warning:
      {
        new(&second._warning) std::string(std::move(tempWarning));
      
        break;
      }
    
      case type::deletion:
      case type::scrub_location:
      {
        second._user_id_and_tweet_id._user_id = tempUserId;
        second._user_id_and_tweet_id._tweet_id = tempTweetId;
      
        break;
      }
    
      case type::limit:
      {
        second._limit = tempLimit;
      
        break;
      }
    
      case type::withhold_status:
      {
        second._withhold_status._user_id = tempUserId;
        second._withhold_status._tweet_id = tempTweetId;
        new(&second._withhold_status._countries) std::vector<std::string>(std::move(tempCountries));
      
        break;
      }
    
      case type::withhold_user:
      {
        second._withhold_user._user_id = tempUserId;
        new(&second._withhold_user._countries) std::vector<std::string>(std::move(tempCountries));
      
        break;
      }
    
      case type::disconnect:
      {
        second._disconnect = tempDisconnectCode;
      
        break;
      }
    
      case type::friends:
      {
        new(&second._friends) std::set<user_id>(std::move(tempFriends));
      
        break;
      }
    
      case type::direct:
      {
        new(&second._direct_message) direct_message(std::move(tempDirectMessage));
      
        break;
      }
    
      case type::invalid:
      case type::unknown:
      {
        break;
      }
    }
  }
  
  const tweet& notification::getTweet() const
  {
    switch (_type)
    {
      case type::tweet:
      {
        return _tweet;
      }
      
      case type::favorite:
      case type::favorited:
      case type::unfavorite:
      case type::unfavorited:
      case type::quoted:
      {
        return _user_and_tweet._tweet;
      }
      
      default:
      {
        assert(false);
      }
    }
  }
  
  const user& notification::getUser() const
  {
    switch (_type)
    {
      case type::update_user:
      case type::block:
      case type::unblock:
      case type::follow:
      case type::followed:
      case type::unfollow:
      {
        return _user;
      }
      
      case type::favorite:
      case type::favorited:
      case type::unfavorite:
      case type::unfavorited:
      case type::quoted:
      {
        return _user_and_tweet._user;
      }
      
      case type::list_add:
      case type::list_added:
      case type::list_remove:
      case type::list_removed:
      case type::list_subscribe:
      case type::list_subscribed:
      case type::list_unsubscribe:
      case type::list_unsubscribed:
      {
        return _user_and_list._user;
      }
      
      default:
      {
        assert(false);
      }
    }
  }
  
  const list& notification::getList() const
  {
    switch (_type)
    {      
      case type::list_created:
      case type::list_destroyed:
      case type::list_updated:
      {
        return _list;
      }
      
      case type::list_add:
      case type::list_added:
      case type::list_remove:
      case type::list_removed:
      case type::list_subscribe:
      case type::list_subscribed:
      case type::list_unsubscribe:
      case type::list_unsubscribed:
      {
        return _user_and_list._list;
      }
      
      default:
      {
        assert(false);
      }
    }
  }
  
  tweet_id notification::getTweetID() const
  {
    switch (_type)
    {
      case type::deletion:
      case type::scrub_location:
      {
        return _user_id_and_tweet_id._tweet_id;
      }
      
      case type::withhold_status:
      {
        return _withhold_status._tweet_id;
      }
      
      default:
      {
        assert(false);
      }
    }
  }
  
  void notification::setTweetID(tweet_id _arg)
  {
    switch (_type)
    {
      case type::deletion:
      case type::scrub_location:
      {
        _user_id_and_tweet_id._tweet_id = _arg;;
      }
      
      case type::withhold_status:
      {
        _withhold_status._tweet_id = _arg;
      }
      
      default:
      {
        assert(false);
      }
    }
  }
  
  user_id notification::getUserID() const
  {
    switch (_type)
    {      
      case type::deletion:
      case type::scrub_location:
      {
        return _user_id_and_tweet_id._user_id;
      }
      
      case type::withhold_status:
      {
        return _withhold_status._user_id;
      }
      
      case type::withhold_user:
      {
        return _withhold_user._user_id;
      }

      default:
      {
        assert(false);
      }
    }
  }
  
  void notification::setUserID(user_id _arg)
  {
    switch (_type)
    {      
      case type::deletion:
      case type::scrub_location:
      {
        _user_id_and_tweet_id._user_id = _arg;
      }
      
      case type::withhold_status:
      {
        _withhold_status._user_id = _arg;
      }
      
      case type::withhold_user:
      {
        _withhold_user._user_id = _arg;
      }

      default:
      {
        assert(false);
      }
    }
  }
  
  const std::vector<std::string>& notification::getCountries() const
  {
    switch (_type)
    { 
      case type::withhold_status:
      {
        return _withhold_status._countries;
      }
      
      case type::withhold_user:
      {
        return _withhold_user._countries;
      }

      default:
      {
        assert(false);
      }
    }
  }
  
  disconnect_code notification::getDisconnectCode() const
  {
    assert(_type == type::disconnect);
    
    return _disconnect;
  }
  
  void notification::setDisconnectCode(disconnect_code _arg)
  {
    assert(_type == type::disconnect);
    
    _disconnect = _arg;
  }
  
  const std::set<user_id>& notification::getFriends() const
  {
    assert(_type == type::friends);
    
    return _friends;
  }
  
  const direct_message& notification::getDirectMessage() const
  {
    assert(_type == type::direct);
    
    return _direct_message;
  }
  
  int notification::getLimit() const
  {
    assert(_type == type::limit);
    
    return _limit;
  }
  
  void notification::setLimit(int _arg)
  {
    assert(_type == type::limit);
    
    _limit = _arg;
  }
  
  const std::string& notification::getWarning() const
  {
    switch (_type)
    {
      case type::stall:
      case type::follow_limit:
      case type::unknown_warning:
      {
        return _warning;
      }
      
      default:
      {
        assert(false);
      }
    }
  }
  
};
