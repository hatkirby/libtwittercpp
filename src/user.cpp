#include "user.h"
#include <json.hpp>
#include "codes.h"
#include "client.h"

namespace twitter {
  
  user::user(const client& tclient, std::string data) try
    : _client(tclient)
  {
    auto json = nlohmann::json::parse(data);
    _id = json["id"].get<user_id>();
    _screen_name = json["screen_name"].get<std::string>();
    _name = json["name"].get<std::string>();
  } catch (const std::invalid_argument& error)
  {
    std::throw_with_nested(malformed_object("user", data));
  } catch (const std::domain_error& error)
  {
    std::throw_with_nested(malformed_object("user", data));
  }
  
  std::set<user_id> user::getFriends() const
  {
    return _client.getFriends(_id);
  }
  
  std::set<user_id> user::getFollowers() const
  {
    return _client.getFollowers(_id);
  }
  
  void user::follow() const
  {
    _client.follow(_id);
  }
  
  void user::unfollow() const
  {
    _client.unfollow(_id);
  }

};
