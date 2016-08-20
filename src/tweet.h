#ifndef TWEET_H_CE980721
#define TWEET_H_CE980721

#include <string>
#include <vector>
#include <utility>
#include <cassert>
#include <list>
#include "user.h"

namespace twitter {
  
  class client;
  
  typedef unsigned long long tweet_id;
  
  class tweet {
    public:
      
      tweet(const client& tclient, std::string data);
      
      tweet(const tweet& other) = delete;
      tweet& operator=(const tweet& other) = delete;
      
      tweet(tweet&& other) = default;
      tweet& operator=(tweet&& other) = default;
      
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

      bool isRetweet() const
      {
        return _is_retweet;
      }
      
      const tweet& getRetweet() const
      {
        assert(_is_retweet);
        
        return *_retweeted_status;
      }
      
      const std::vector<std::pair<user_id, std::string>>& getMentions() const
      {
        return _mentions;
      }
      
      std::string generateReplyPrefill() const;
      
      tweet reply(std::string message, std::list<long> media_ids = {}) const;
      
      bool isMyTweet() const;
      
      std::string getURL() const;
      
    private:
      
      const client& _client;
      tweet_id _id;
      std::string _text;
      std::unique_ptr<user> _author;
      bool _is_retweet = false;
      std::unique_ptr<tweet> _retweeted_status;
      std::vector<std::pair<user_id, std::string>> _mentions;
  };
  
};

#endif /* end of include guard: TWEET_H_CE980721 */
