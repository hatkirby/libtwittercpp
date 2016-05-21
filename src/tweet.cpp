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
  
  tweet::operator bool() const
  {
    return _valid;
  }
  
};
