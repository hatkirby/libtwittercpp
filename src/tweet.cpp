#include "tweet.h"
#include <json.hpp>
#include "util.h"
#include "codes.h"
#include "client.h"

namespace twitter {
  
  tweet::tweet(std::string data) try
    : _valid(true)
  {
    auto json = nlohmann::json::parse(data);
    _id = json["id"].get<tweet_id>();
    _text = json["text"].get<std::string>();
    _author = make_unique<user>(json["user"].dump());
    
    if (!json["retweeted_status"].is_null())
    {
      _is_retweet = true;
      
      _retweeted_status = make_unique<tweet>(json["retweeted_status"].dump());
    }
    
    if (!json["entities"].is_null())
    {
      auto entities = json["entities"];
      if (!entities["user_mentions"].is_null())
      {
        for (auto mention : entities["user_mentions"])
        {
          _mentions.push_back(std::make_pair(mention["id"].get<user_id>(), mention["screen_name"].get<std::string>()));
        }
      }
    }
  } catch (const std::invalid_argument& error)
  {
    std::throw_with_nested(malformed_object("tweet", data));
  } catch (const std::domain_error& error)
  {
    std::throw_with_nested(malformed_object("tweet", data));
  }
  
  tweet::tweet(const tweet& other)
  {
    _valid = other._valid;
    _id = other._id;
    _text = other._text;
    _author = make_unique<user>(*other._author);
    _is_retweet = other._is_retweet;
    
    if (_is_retweet)
    {
      _retweeted_status = make_unique<tweet>(*other._retweeted_status);
    }
    
    _mentions = other._mentions;
  }
  
  tweet::tweet(tweet&& other) : tweet()
  {
    swap(*this, other);
  }
  
  tweet& tweet::operator=(tweet other)
  {
    swap(*this, other);
    
    return *this;
  }
  
  void swap(tweet& first, tweet& second)
  {
    std::swap(first._valid, second._valid);
    std::swap(first._id, second._id);
    std::swap(first._text, second._text);
    std::swap(first._author, second._author);
    std::swap(first._is_retweet, second._is_retweet);
    std::swap(first._retweeted_status, second._retweeted_status);
    std::swap(first._mentions, second._mentions);
  }
  
  std::string tweet::generateReplyPrefill(const user& me) const
  {
    std::ostringstream output;
    output << "@" << _author->getScreenName() << " ";
    
    for (auto mention : _mentions)
    {
      if ((mention.first != _author->getID()) && (mention.first != me.getID()))
      {
        output << "@" << mention.second << " ";
      }
    }
    
    return output.str();
  }
  
  std::string tweet::getURL() const
  {
    assert(_valid);
    
    std::ostringstream urlstr;
    urlstr << "https://twitter.com/";
    urlstr << _author->getScreenName();
    urlstr << "/statuses/";
    urlstr << _id;
    
    return urlstr.str();
  }
  
};
