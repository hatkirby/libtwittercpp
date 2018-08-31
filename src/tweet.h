#ifndef TWEET_H_CE980721
#define TWEET_H_CE980721

#include <string>
#include <vector>
#include <utility>
#include <ctime>
#include "../vendor/hkutil/hkutil/recptr.h"
#include "user.h"

namespace twitter {

  typedef unsigned long long tweet_id;

  class tweet {
  public:

    tweet(std::string data);

    tweet_id getID() const
    {
      return _id;
    }

    std::string getText() const
    {
      return _text;
    }

    const user& getAuthor() const
    {
      return *_author;
    }

    const std::time_t& getCreatedAt() const
    {
      return _created_at;
    }

    bool isRetweet() const
    {
      return _is_retweet;
    }

    const tweet& getRetweet() const
    {
      if (!_is_retweet)
      {
        throw std::logic_error("Tweet is not a retweet");
      }

      return *_retweeted_status;
    }

    const std::vector<std::pair<user_id, std::string>>& getMentions() const
    {
      return _mentions;
    }

    std::string generateReplyPrefill(const user& me) const;

    std::string getURL() const;

  private:

    tweet_id _id;
    std::string _text;
    hatkirby::recptr<user> _author;
    std::time_t _created_at;
    bool _is_retweet = false;
    hatkirby::recptr<tweet> _retweeted_status;
    std::vector<std::pair<user_id, std::string>> _mentions;
  };

};

#endif /* end of include guard: TWEET_H_CE980721 */
