#ifndef TWEET_H_CE980721
#define TWEET_H_CE980721

#include <string>
#include "user.h"
#include <vector>
#include <utility>

namespace twitter {
  
  typedef unsigned long long tweet_id;
  
  class tweet {
    public:
      tweet();
      tweet(std::string data);
      tweet(const tweet& other);
      tweet(tweet&& other);
      ~tweet();
      
      tweet& operator=(tweet other);
      friend void swap(tweet& first, tweet& second);
      
      tweet_id getID() const;
      std::string getText() const;
      const user& getAuthor() const;
      bool isRetweet() const;
      tweet getRetweet() const;
      std::vector<std::pair<user_id, std::string>> getMentions() const;
      std::string getURL() const;
      
      operator bool() const;
      
    private:
      bool _valid;
      tweet_id _id;
      std::string _text;
      user _author;
      bool _is_retweet = false;
      tweet* _retweeted_status = nullptr;
      std::vector<std::pair<user_id, std::string>> _mentions;
  };
  
};

#endif /* end of include guard: TWEET_H_CE980721 */
