#ifndef TWEET_H_CE980721
#define TWEET_H_CE980721

#include <string>
#include <vector>
#include <utility>
#include <cassert>
#include <list>
#include <memory>
#include <ctime>
#include "user.h"

namespace twitter {

  class client;

  typedef unsigned long long tweet_id;

  class tweet {
    public:

      tweet() {}
      tweet(std::string data);

      tweet(const tweet& other);
      tweet(tweet&& other);

      tweet& operator=(tweet other);

      friend void swap(tweet& first, tweet& second);

      tweet_id getID() const
      {
        assert(_valid);

        return _id;
      }

      std::string getText() const
      {
        assert(_valid);

        return _text;
      }

      const user& getAuthor() const
      {
        assert(_valid);

        return *_author;
      }

      const std::time_t& getCreatedAt() const
      {
        assert(_valid);

        return _created_at;
      }

      bool isRetweet() const
      {
        assert(_valid);

        return _is_retweet;
      }

      const tweet& getRetweet() const
      {
        assert(_valid && _is_retweet);

        return *_retweeted_status;
      }

      const std::vector<std::pair<user_id, std::string>>& getMentions() const
      {
        assert(_valid);

        return _mentions;
      }

      std::string generateReplyPrefill(const user& me) const;

      std::string getURL() const;

    private:

      bool _valid = false;
      tweet_id _id;
      std::string _text;
      std::unique_ptr<user> _author;
      std::time_t _created_at;
      bool _is_retweet = false;
      std::unique_ptr<tweet> _retweeted_status;
      std::vector<std::pair<user_id, std::string>> _mentions;
  };

};

#endif /* end of include guard: TWEET_H_CE980721 */
