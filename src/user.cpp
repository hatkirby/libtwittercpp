#include "user.h"
#include <json.hpp>

using nlohmann::json;

namespace twitter {
  
  user::user() : _valid(false)
  {
    
  }
  
  user::user(std::string data) : _valid(true)
  {
    auto _data = json::parse(data);
    _id = _data.at("id");
    _screen_name = _data.at("screen_name");
    _name = _data.at("name");
  }
  
  user_id user::getID() const
  {
    return _id;
  }
  
  std::string user::getScreenName() const
  {
    return _screen_name;
  }
  
  std::string user::getName() const
  {
    return _name;
  }
  
  user::operator bool() const
  {
    return _valid;
  }
  
  bool user::operator==(const user& other) const
  {
    return _id == other._id;
  }
  
  bool user::operator!=(const user& other) const
  {
    return _id != other._id;
  }

};
