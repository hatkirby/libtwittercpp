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
    _retweeted = _data.at("retweeted");
    
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
    
    return _retweeted;
  }
  
  std::vector<std::pair<user_id, std::string>> tweet::getMentions() const
  {
    return _mentions;
  }
  
  tweet::operator bool() const
  {
    return _valid;
  }
  
};
