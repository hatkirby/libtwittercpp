#include "timeline.h"
#include <sstream>
#include <json.hpp>
#include <hkutil/string.h>
#include "codes.h"
#include "request.h"

namespace twitter {

  timeline::timeline(
    const auth& tauth,
    std::string url) :
      auth_(tauth),
      url_(std::move(url))
  {
  }

  std::list<tweet> timeline::poll()
  {
    tweet_id maxId;
    std::list<tweet> result;

    for (int i = 0; i < 5; i++)
    {
      std::ostringstream urlstr;
      urlstr << url_;

      std::list<std::string> arguments;

      if (i > 0)
      {
        arguments.push_back("max_id=" + std::to_string(maxId));
      }

      if (hasSince_)
      {
        arguments.push_back("since_id=" + std::to_string(sinceId_));
      }

      if (!arguments.empty())
      {
        urlstr << "?";
        urlstr << hatkirby::implode(
          std::begin(arguments), std::end(arguments), "&");
      }

      std::string theUrl = urlstr.str();
      std::string response = get(auth_, theUrl).perform();

      try
      {
        nlohmann::json rjs = nlohmann::json::parse(response);

        if (rjs.empty())
        {
          break;
        }

        for (auto& single : rjs)
        {
          result.emplace_back(single.dump());
        }
      } catch (const std::invalid_argument& error)
      {
        std::throw_with_nested(invalid_response(response));
      } catch (const std::domain_error& error)
      {
        std::throw_with_nested(invalid_response(response));
      }

      maxId = result.back().getID() - 1;
    }

    if (!result.empty())
    {
      sinceId_ = result.front().getID();
      hasSince_ = true;
    }

    return result;
  }

};
