#include "tweet.h"
#include <json.hpp>
#include "util.h"
#include "codes.h"
#include "client.h"

namespace twitter {
  
  tweet::tweet(const client& tclient, std::string data) try
    : _client(tclient)
  {
    auto json = nlohmann::json::parse(data);
    _id = json["id"].get<tweet_id>();
    _text = json["text"].get<std::string>();
    _author = make_unique<user>(_client, json["user"].dump());
    
    if (!json["retweeted_status"].is_null())
    {
      _is_retweet = true;
      
      _retweeted_status = make_unique<tweet>(_client, json["retweeted_status"].dump());
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
  
  std::string tweet::generateReplyPrefill() const
  {
    std::ostringstream output;
    output << "@" << _author->getScreenName() << " ";
    
    for (auto mention : _mentions)
    {
      if ((mention.first != _author->getID()) && (mention.first != _client.getUser().getID()))
      {
        output << "@" << mention.second << " ";
      }
    }
    
    return output.str();
  }
  
  tweet tweet::reply(std::string message, std::list<long> media_ids) const
  {
    return _client.replyToTweet(message, _id, media_ids);
  }
  
  bool tweet::isMyTweet() const
  {
    return *_author == _client.getUser();
  }
  
};
