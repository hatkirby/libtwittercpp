#include "tweet.h"
#include <json.hpp>
#include <cassert>

using nlohmann::json;

namespace twitter {
  
  tweet::tweet() : _valid(false)
  {
    
  }
  
  tweet::tweet(std::string data) : _valid(true)
  {
    auto _data = json::parse(data);
    _id = _data.at("id");
    _text = _data.at("text");
    _author = user(_data.at("user").dump());
    
    if (_data.find("retweeted_status") != _data.end())
    {
      _is_retweet = true;
      
      std::string retweet = _data.at("retweeted_status").dump();
      _retweeted_status = new tweet(retweet);
    }
    
    if (_data.find("entities") != _data.end())
    {
      auto _entities = _data.at("entities");
      if (_entities.find("user_mentions") != _entities.end())
      {
        for (auto _mention : _entities.at("user_mentions"))
        {
          _mentions.push_back(std::make_pair(_mention.at("id"), _mention.at("screen_name").get<std::string>()));
        }
      }
    }
  }
  
  tweet::tweet(const tweet& other)
  {
    _valid = other._valid;
    _id = other._id;
    _text = other._text;
    _author = other._author;
    _is_retweet = other._is_retweet;
    
    if (_is_retweet)
    {
      _retweeted_status = new tweet(*other._retweeted_status);
    }
    
    _mentions = other._mentions;
  }
  
  tweet::tweet(tweet&& other) : tweet()
  {
    swap(*this, other);
  }
  
  tweet::~tweet()
  {
    if (_is_retweet)
    {
      delete _retweeted_status;
    }
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
  
  tweet_id tweet::getID() const
  {
    assert(_valid);
    
    return _id;
  }
  
  std::string tweet::getText() const
  {
    assert(_valid);
    
    return _text;
  }
  
  const user& tweet::getAuthor() const
  {
    assert(_valid);
    
    return _author;
  }
  
  bool tweet::isRetweet() const
  {
    assert(_valid);
    
    return _is_retweet;
  }
  
  tweet tweet::getRetweet() const
  {
    assert(_valid && _is_retweet);
    
    return *_retweeted_status;
  }
  
  std::vector<std::pair<user_id, std::string>> tweet::getMentions() const
  {
    return _mentions;
  }
  
  std::string tweet::getURL() const
  {
    return "https://twitter.com/" + _author.getScreenName() + "/statuses/" + std::to_string(_id);
  }
  
  tweet::operator bool() const
  {
    return _valid;
  }
  
};
