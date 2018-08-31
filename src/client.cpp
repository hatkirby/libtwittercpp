#include "client.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <json.hpp>
#include <thread>
#include <hkutil/string.h>
#include "request.h"

namespace twitter {

  client::client(
    const auth& _arg) :
      auth_(_arg),
      currentUser_(
        get(auth_,
          "https://api.twitter.com/1.1/account/verify_credentials.json")
            .perform())
  {
  }

  tweet client::updateStatus(std::string msg, std::list<long> media_ids) const
  {
    std::stringstream datastrstream;
    datastrstream << "status=" << OAuth::PercentEncode(msg);

    if (!media_ids.empty())
    {
      datastrstream << "&media_ids=";
      datastrstream << hatkirby::implode(std::begin(media_ids), std::end(media_ids), ",");
    }

    return tweet(
      post(auth_,
        "https://api.twitter.com/1.1/statuses/update.json",
        datastrstream.str())
      .perform());
  }

  tweet client::replyToTweet(std::string msg, tweet_id in_response_to, std::list<long> media_ids) const
  {
    std::stringstream datastrstream;
    datastrstream << "status=" << OAuth::PercentEncode(msg);
    datastrstream << "&in_reply_to_status_id=";
    datastrstream << in_response_to;

    if (!media_ids.empty())
    {
      datastrstream << "&media_ids=";
      datastrstream << hatkirby::implode(std::begin(media_ids), std::end(media_ids), ",");
    }

    return tweet(
      post(auth_,
        "https://api.twitter.com/1.1/statuses/update.json",
        datastrstream.str())
      .perform());
  }

  tweet client::replyToTweet(std::string msg, const tweet& in_response_to, std::list<long> media_ids) const
  {
    return replyToTweet(msg, in_response_to.getID(), media_ids);
  }

  long client::uploadMedia(std::string media_type, const char* data, long data_length) const try
  {
    curl::curl_form form;
    std::string str_data_length = std::to_string(data_length);

    curl::curl_pair<CURLformoption, std::string> command_name(CURLFORM_COPYNAME, "command");
    curl::curl_pair<CURLformoption, std::string> command_cont(CURLFORM_COPYCONTENTS, "INIT");
    curl::curl_pair<CURLformoption, std::string> bytes_name(CURLFORM_COPYNAME, "total_bytes");
    curl::curl_pair<CURLformoption, std::string> bytes_cont(CURLFORM_COPYCONTENTS, str_data_length);
    curl::curl_pair<CURLformoption, std::string> type_name(CURLFORM_COPYNAME, "media_type");
    curl::curl_pair<CURLformoption, std::string> type_cont(CURLFORM_COPYCONTENTS, media_type);
    form.add(command_name, command_cont);
    form.add(bytes_name, bytes_cont);
    form.add(type_name, type_cont);

    if (media_type == "image/gif")
    {
      curl::curl_pair<CURLformoption, std::string> category_name(CURLFORM_COPYNAME, "media_category");
      curl::curl_pair<CURLformoption, std::string> category_cont(CURLFORM_COPYCONTENTS, "tweet_gif");
      form.add(category_name, category_cont);
    }

    std::string init_response =
      multipost(auth_,
        "https://upload.twitter.com/1.1/media/upload.json",
        form.get())
      .perform();

    long media_id;

    try
    {
      nlohmann::json response_json = nlohmann::json::parse(init_response);
      media_id = response_json["media_id"].get<long>();
    } catch (const std::invalid_argument& error)
    {
      std::throw_with_nested(invalid_response(init_response));
    } catch (const std::domain_error& error)
    {
      std::throw_with_nested(invalid_response(init_response));
    }

    // TODO: Currently have to use the C libcurl API to create this form because it uses a buffer and
    // libcurlcpp currently messes that up.
    curl_httppost* append_form_post = nullptr;
    curl_httppost* append_form_last = nullptr;
    if ( curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "command", CURLFORM_COPYCONTENTS, "APPEND", CURLFORM_END)
      || curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "media_id", CURLFORM_COPYCONTENTS, std::to_string(media_id).c_str(), CURLFORM_END)
      || curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "media", CURLFORM_BUFFER, "media", CURLFORM_BUFFERPTR, data, CURLFORM_BUFFERLENGTH, data_length, CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END)
      || curl_formadd(&append_form_post, &append_form_last, CURLFORM_COPYNAME, "segment_index", CURLFORM_COPYCONTENTS, std::to_string(0).c_str(), CURLFORM_END))
    {
      assert(false);
    }

    multipost(auth_, "https://upload.twitter.com/1.1/media/upload.json", append_form_post).perform();

    curl_formfree(append_form_post);

    curl::curl_form finalize_form;
    std::string str_media_id = std::to_string(media_id);

    curl::curl_pair<CURLformoption, std::string> command3_name(CURLFORM_COPYNAME, "command");
    curl::curl_pair<CURLformoption, std::string> command3_cont(CURLFORM_COPYCONTENTS, "FINALIZE");
    curl::curl_pair<CURLformoption, std::string> media_id_name(CURLFORM_COPYNAME, "media_id");
    curl::curl_pair<CURLformoption, std::string> media_id_cont(CURLFORM_COPYCONTENTS, str_media_id);
    finalize_form.add(command3_name, command3_cont);
    finalize_form.add(media_id_name, media_id_cont);

    std::string finalize_response =
      multipost(auth_,
        "https://upload.twitter.com/1.1/media/upload.json",
        finalize_form.get())
      .perform();

    nlohmann::json finalize_json;

    try
    {
      finalize_json = nlohmann::json::parse(finalize_response);
    } catch (const std::invalid_argument& error)
    {
      std::throw_with_nested(invalid_response(finalize_response));
    }

    if (finalize_json.find("processing_info") != finalize_json.end())
    {
      std::stringstream datastr;
      datastr << "https://upload.twitter.com/1.1/media/upload.json?command=STATUS&media_id=" << media_id;

      for (;;)
      {
        std::string status_response = get(auth_, datastr.str()).perform();

        try
        {
          nlohmann::json status_json = nlohmann::json::parse(status_response);
          std::string state = status_json["processing_info"]["state"].get<std::string>();

          if (state == "succeeded")
          {
            break;
          }

          int ttw = status_json["processing_info"]["check_after_secs"].get<int>();
          std::this_thread::sleep_for(std::chrono::seconds(ttw));
        } catch (const std::invalid_argument& error)
        {
          std::throw_with_nested(invalid_response(status_response));
        } catch (const std::domain_error& error)
        {
          std::throw_with_nested(invalid_response(status_response));
        }
      }
    }

    return media_id;
  } catch (const curl::curl_exception& error)
  {
    std::throw_with_nested(connection_error());
  }

  std::set<user_id> client::getFriends(user_id id) const
  {
    long long cursor = -1;
    std::set<user_id> result;

    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/friends/ids.json?user_id=";
      urlstream << id;
      urlstream << "&cursor=";
      urlstream << cursor;

      std::string url = urlstream.str();
      std::string response_data = get(auth_, url).perform();

      try
      {
        nlohmann::json rjs = nlohmann::json::parse(response_data);

        cursor = rjs["next_cursor"].get<long long>();
        result.insert(std::begin(rjs["ids"]), std::end(rjs["ids"]));
      } catch (const std::invalid_argument& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      } catch (const std::domain_error& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      }
    }

    return result;
  }

  std::set<user_id> client::getFriends(const user& id) const
  {
    return getFriends(id.getID());
  }

  std::set<user_id> client::getFriends() const
  {
    return getFriends(getUser().getID());
  }

  std::set<user_id> client::getFollowers(user_id id) const
  {
    long long cursor = -1;
    std::set<user_id> result;

    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/followers/ids.json?user_id=";
      urlstream << id;
      urlstream << "&cursor=";
      urlstream << cursor;

      std::string url = urlstream.str();
      std::string response_data = get(auth_, url).perform();

      try
      {
        nlohmann::json rjs = nlohmann::json::parse(response_data);

        cursor = rjs["next_cursor"].get<long long>();
        result.insert(std::begin(rjs["ids"]), std::end(rjs["ids"]));
      } catch (const std::invalid_argument& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      } catch (const std::domain_error& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      }
    }

    return result;
  }

  std::set<user_id> client::getFollowers(const user& id) const
  {
    return getFollowers(id.getID());
  }

  std::set<user_id> client::getFollowers() const
  {
    return getFollowers(getUser().getID());
  }

  std::set<user_id> client::getBlocks() const
  {
    long long cursor = -1;
    std::set<user_id> result;

    while (cursor != 0)
    {
      std::stringstream urlstream;
      urlstream << "https://api.twitter.com/1.1/blocks/ids.json?cursor=";
      urlstream << cursor;

      std::string url = urlstream.str();
      std::string response_data = get(auth_, url).perform();

      try
      {
        nlohmann::json rjs = nlohmann::json::parse(response_data);

        cursor = rjs["next_cursor"].get<long long>();
        result.insert(std::begin(rjs["ids"]), std::end(rjs["ids"]));
      } catch (const std::invalid_argument& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      } catch (const std::domain_error& error)
      {
        std::throw_with_nested(invalid_response(response_data));
      }
    }

    return result;
  }

  void client::follow(user_id toFollow) const
  {
    std::stringstream datastrstream;
    datastrstream << "follow=true&user_id=";
    datastrstream << toFollow;

    post(auth_, "https://api.twitter.com/1.1/friendships/create.json", datastrstream.str()).perform();
  }

  void client::follow(const user& toFollow) const
  {
    return follow(toFollow.getID());
  }

  void client::unfollow(user_id toUnfollow) const
  {
    std::stringstream datastrstream;
    datastrstream << "user_id=";
    datastrstream << toUnfollow;

    post(auth_, "https://api.twitter.com/1.1/friendships/destroy.json", datastrstream.str()).perform();
  }

  void client::unfollow(const user& toUnfollow) const
  {
    return unfollow(toUnfollow.getID());
  }

  const user& client::getUser() const
  {
    return currentUser_;
  }

  const configuration& client::getConfiguration() const
  {
    if (!_configuration || (difftime(time(NULL), _last_configuration_update) > 60*60*24))
    {
      _configuration =
        std::make_unique<configuration>(
          get(auth_,
            "https://api.twitter.com/1.1/help/configuration.json")
          .perform());

      _last_configuration_update = time(NULL);
    }

    return *_configuration;
  }

  std::list<tweet> client::hydrateTweets(std::set<tweet_id> ids) const
  {
    std::list<tweet> result;

    while (!ids.empty())
    {
      std::set<tweet_id> cur;

      for (int i = 0; i < 100 && !ids.empty(); i++)
      {
        cur.insert(*std::begin(ids));
        ids.erase(std::begin(ids));
      }

      std::string datastr = "id=" +
        OAuth::PercentEncode(
          hatkirby::implode(std::begin(cur), std::end(cur), ","));

      std::string response =
        post(auth_,
          "https://api.twitter.com/1.1/statuses/lookup.json",
          datastr).perform();

      nlohmann::json rjs = nlohmann::json::parse(response);

      for (auto& single : rjs)
      {
        result.emplace_back(single.dump());
      }
    }

    return result;
  }

  std::list<user> client::hydrateUsers(std::set<user_id> ids) const
  {
    std::list<user> result;

    while (!ids.empty())
    {
      std::set<user_id> cur;

      for (int i = 0; i < 100 && !ids.empty(); i++)
      {
        cur.insert(*std::begin(ids));
        ids.erase(std::begin(ids));
      }

      std::string datastr = "user_id=" +
        OAuth::PercentEncode(
          hatkirby::implode(std::begin(cur), std::end(cur), ","));

      std::string response =
        post(auth_,
          "https://api.twitter.com/1.1/users/lookup.json",
          datastr).perform();

      nlohmann::json rjs = nlohmann::json::parse(response);

      for (auto& single : rjs)
      {
        result.emplace_back(single.dump());
      }
    }

    return result;
  }

};
