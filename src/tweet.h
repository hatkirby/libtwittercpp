#ifndef TWEET_H_CE980721
#define TWEET_H_CE980721

#include <string>
#include "user.h"

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
      
      operator bool() const;
      
    private:
      bool _valid;
      tweet_id _id;
      std::string _text;
      user _author;
      bool _retweeted;
  };
  
};

#endif /* end of include guard: TWEET_H_CE980721 */
