#include "tweet.h"
#include <json.hpp>
#include <sstream>
#include "util.h"
#include "codes.h"
#include "client.h"

namespace twitter {

  tweet::tweet(std::string data)
  {
    try
    {
      auto json = nlohmann::json::parse(data);
      _id = json["id"].get<tweet_id>();
      _text = json["text"].get<std::string>();
      _author = new user(json["user"].dump());

      std::tm ctt = { 0 };
      std::stringstream createdAtStream;
      createdAtStream << json["created_at"].get<std::string>();
      createdAtStream >> std::get_time(&ctt, "%a %b %d %H:%M:%S +0000 %Y");
      _created_at = twitter::timegm(&ctt);

      if (!json["retweeted_status"].is_null())
      {
        _is_retweet = true;

        _retweeted_status = new tweet(json["retweeted_status"].dump());
      }

      if (!json["entities"].is_null())
      {
        auto entities = json["entities"];
        if (!entities["user_mentions"].is_null())
        {
          for (auto mention : entities["user_mentions"])
          {
            _mentions.emplace_back(
              mention["id"].get<user_id>(),
              mention["screen_name"].get<std::string>());
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
    std::ostringstream urlstr;
    urlstr << "https://twitter.com/";
    urlstr << _author->getScreenName();
    urlstr << "/statuses/";
    urlstr << _id;

    return urlstr.str();
  }

};
