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
      
      tweet_id getID() const;
      std::string getText() const;
      const user& getAuthor() const;
      bool isRetweet() const;
      std::vector<std::pair<user_id, std::string>> getMentions() const;
      
      operator bool() const;
      
    private:
      bool _valid;
      tweet_id _id;
      std::string _text;
      user _author;
      bool _retweeted;
      std::vector<std::pair<user_id, std::string>> _mentions;
  };
  
};

#endif /* end of include guard: TWEET_H_CE980721 */
