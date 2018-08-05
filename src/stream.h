#ifndef STREAM_H_E9146952
#define STREAM_H_E9146952

#include <functional>
#include <list>
#include <string>
#include <chrono>
#include <thread>
#include "bounding_box.h"
#include "auth.h"
#include "user.h"

namespace twitter {

  class notification;

  class
  [[deprecated("The Twitter streaming API will sunset on August 16th, 2018")]]
  stream {
  public:

    typedef std::function<void(notification _notification)> notify_callback;

    stream(
      const auth& tauth,
      notify_callback callback,
      bool with_followings = true,
      bool receive_all_replies = false,
      std::list<std::string> track = {},
      std::list<bounding_box> locations = {});

    ~stream();

    stream(const stream& other) = delete;
    stream(stream&& other) = delete;
    stream& operator=(const stream& other) = delete;
    stream& operator=(stream&& other) = delete;

  private:
    enum class backoff {
      none,
      network,
      http,
      rate_limit
    };

    static std::string generateUrl(
      bool with_followings,
      bool receive_all_replies,
      std::list<std::string> track,
      std::list<bounding_box> locations);

    void run(std::string url);
    int progress();
    size_t write(char* ptr, size_t size, size_t nmemb);

    const auth& _auth;
    notify_callback _notify;
    bool _stop = false;
    std::string _buffer;
    time_t _last_write;
    bool _established = false;
    backoff _backoff_type = backoff::none;
    std::chrono::milliseconds _backoff_amount;
    user _currentUser;
    std::thread _thread;
  };

}

#endif /* end of include guard: STREAM_H_E9146952 */
