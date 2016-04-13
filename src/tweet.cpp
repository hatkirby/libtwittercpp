#include "tweet.h"

namespace twitter {
  
  tweet::tweet()
  {
    _valid = false;
  }
  
  tweet::tweet(const json& data)
  {
    _valid = true;
  }
  
};
