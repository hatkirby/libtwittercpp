#include "notification.h"
#include <cassert>
#include <new>
#include <json.hpp>

using nlohmann::json;

namespace twitter {
  
  notification::type notification::getType() const
  {
    return _type;
  }
  
  notification::notification() : _type(type::invalid)
  {
    
  }
  
  notification::notification(std::string data, const user& current_user)
  {
    try {
      auto _data = json::parse(data);
    
      if (_data.find("in_reply_to_status_id") != _data.end())
      {
        _type = type::tweet;
      
        new(&_tweet) tweet(data);
      } else if (_data.find("event") != _data.end())
      {
        std::string event = _data.at("event");
        user source(_data.at("source").dump());
        user target(_data.at("target").dump());
      
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
          new(&_user_and_tweet._tweet) tweet(_data.at("target_object").dump());
        
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
          new(&_user_and_tweet._tweet) tweet(_data.at("target_object").dump());
        
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
        
          new(&_list) list(_data.at("target_object").dump());
        } else if (event == "list_destroyed")
        {
          _type = type::list_destroyed;
        
          new(&_list) list(_data.at("target_object").dump());
        } else if (event == "list_updated")
        {
          _type = type::list_updated;
        
          new(&_list) list(_data.at("target_object").dump());
        } else if (event == "list_member_added")
        {
          new(&_user_and_list._list) list(_data.at("target_object").dump());
        
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
          new(&_user_and_list._list) list(_data.at("target_object").dump());
        
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
          new(&_user_and_list._list) list(_data.at("target_object").dump());
        
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
          new(&_user_and_list._list) list(_data.at("target_object").dump());
        
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
          new(&_user_and_tweet._tweet) tweet(_data.at("target_object").dump());
        }
      } else if (_data.find("warning") != _data.end())
      {
        new(&_warning) std::string(_data.at("warning").at("message").get<std::string>());
      
        if (_data.at("warning").at("code") == "FALLING_BEHIND")
        {
          _type = type::stall;
        } else if (_data.at("warning").at("code") == "FOLLOWS_OVER_LIMIT")
        {
          _type = type::follow_limit;
        } else {
          _type = type::unknown_warning;
        }
      } else if (_data.find("delete") != _data.end())
      {
        _type = type::deletion;
      
        _user_id_and_tweet_id._tweet_id = _data.at("delete").at("status").at("id");
        _user_id_and_tweet_id._user_id = _data.at("delete").at("status").at("user_id");
      } else if (_data.find("scrub_geo") != _data.end())
      {
        _type = type::scrub_location;
      
        _user_id_and_tweet_id._tweet_id = _data.at("scrub_geo").at("up_to_status_id");
        _user_id_and_tweet_id._user_id = _data.at("scrub_geo").at("user_id");
      } else if (_data.find("limit") != _data.end())
      {
        _type = type::limit;
      
        _limit = _data.at("limit").at("track");
      } else if (_data.find("status_withheld") != _data.end())
      {
        _type = type::withhold_status;
      
        _withhold_status._user_id = _data.at("status_withheld").at("user_id");
        _withhold_status._tweet_id = _data.at("status_withheld").at("id");
      
        new(&_withhold_status._countries) std::vector<std::string>();
        for (auto s : _data.at("status_withheld").at("withheld_in_countries"))
        {
          _withhold_status._countries.push_back(s);
        }
      } else if (_data.find("user_withheld") != _data.end())
      {
        _type = type::withhold_user;
      
        _withhold_user._user_id = _data.at("user_withheld").at("id");
      
        new(&_withhold_user._countries) std::vector<std::string>();
        for (auto s : _data.at("user_withheld").at("withheld_in_countries"))
        {
          _withhold_user._countries.push_back(s);
        }
      } else if (_data.find("disconnect") != _data.end())
      {
        _type = type::disconnect;
      
        switch (_data.at("disconnect").at("code").get<int>())
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
      } else if (_data.find("friends") != _data.end())
      {
        _type = type::friends;
      
        new(&_friends) std::set<user_id>(_data.at("friends").begin(), _data.at("friends").end());
      } else if (_data.find("direct_message") != _data.end())
      {
        _type = type::direct;
      
        new(&_direct_message) direct_message(_data.at("direct_message").dump());
      } else {
        _type = type::unknown;
      }
    } catch (std::invalid_argument e)
    {
      _type = type::invalid;
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
    }
  }
  
  notification& notification::operator=(const notification& other)
  {
    this->~notification();
    
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
    }
    
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
    }
  }
  
  tweet notification::getTweet() const
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
        
        return tweet();
      }
    }
  }
  
  user notification::getUser() const
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
        
        return user();
      }
    }
  }
  
  list notification::getList() const
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
        
        return list();
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
        
        return 0;
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
        
        return 0;
      }
    }
  }
  
  std::vector<std::string> notification::getCountries() const
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
        
        return std::vector<std::string>();
      }
    }
  }
  
  disconnect_code notification::getDisconnectCode() const
  {
    assert(_type == type::disconnect);
    
    return _disconnect;
  }
  
  std::set<user_id> notification::getFriends() const
  {
    assert(_type == type::friends);
    
    return _friends;
  }
  
  direct_message notification::getDirectMessage() const
  {
    assert(_type == type::direct);
    
    return _direct_message;
  }
  
  int notification::getLimit() const
  {
    assert(_type == type::limit);
    
    return _limit;
  }
  
  std::string notification::getWarning() const
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
        
        return "";
      }
    }
  }
  
  notification::operator bool() const
  {
    return _type != type::invalid;
  }
  
};
