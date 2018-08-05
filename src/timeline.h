#ifndef TIMELINE_H_D359681C
#define TIMELINE_H_D359681C

#include <functional>
#include <list>
#include <string>
#include "auth.h"
#include "tweet.h"

namespace twitter {

  class timeline {
  public:

    timeline(
      const auth& tauth,
      std::string url);

    std::list<tweet> poll();

  private:

    const auth& auth_;
    std::string url_;
    bool hasSince_ = false;
    tweet_id sinceId_;
  };

}

#endif /* end of include guard: TIMELINE_H_D359681C */
