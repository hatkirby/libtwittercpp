#include "tweet.h"
#include <json.hpp>

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
  }
  
  tweet_id tweet::getID() const
  {
    return _id;
  }
  
  std::string tweet::getText() const
  {
    return _text;
  }
  
  const user& tweet::getAuthor() const
  {
    return _author;
  }
  
  tweet::operator bool() const
  {
    return _valid;
  }
  
};
