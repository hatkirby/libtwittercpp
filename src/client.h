#ifndef TWITTER_H_ABFF6A12
#define TWITTER_H_ABFF6A12

#include "codes.h"
#include "tweet.h"
#include "auth.h"
#include <list>
#include <thread>
#include <mutex>
#include "notification.h"
#include <set>
#include <ctime>
#include <chrono>

namespace OAuth {
  class Consumer;
  class Token;
  class Client;
};

class curl_httppost;

namespace twitter {
  
  class client {
    public:
      class stream {
        public:
          typedef std::function<void(notification _notification)> notify_callback;
          
          stream(client& _client);
          
          void setNotifyCallback(notify_callback _n);
          
          bool isRunning() const;
          void start();
          void stop();
          
          int progress();
          size_t write(char* ptr, size_t size, size_t nmemb);
          
        private:
          enum class backoff {
            none,
            network,
            http,
            rate_limit
          };
          
          void run();
          
          client& _client;
          notify_callback _notify;
          bool _stop = false;
          std::thread _thread;
          std::mutex _running_mutex;
          std::mutex _notify_mutex;
          std::mutex _stall_mutex;
          std::string _buffer;
          time_t _last_write;
          bool _established = false;
          backoff _backoff_type = backoff::none;
          std::chrono::milliseconds _backoff_amount;
      };
      
      client(const auth& _auth);
      ~client();
      
      response updateStatus(std::string msg, tweet& result, tweet in_response_to = tweet(), std::list<long> media_ids = {});
      response uploadMedia(std::string media_type, const char* data, long data_length, long& media_id);
      
      response follow(user_id toFollow);
      response follow(user toFollow);
      
      response unfollow(user_id toUnfollow);
      response unfollow(user toUnfollow);
      
      response getFriends(std::set<user_id>& result);
      response getFollowers(std::set<user_id>& result);
      
      const user& getUser() const;
      
      void setUserStreamNotifyCallback(stream::notify_callback callback);
      void startUserStream();
      void stopUserStream();
      
      std::string generateReplyPrefill(tweet t) const;
      
    private:
      friend class stream;
      
      OAuth::Consumer* _oauth_consumer;
      OAuth::Token* _oauth_token;
      OAuth::Client* _oauth_client;
      
      user _current_user;
      stream _user_stream{*this};
      
      bool performGet(std::string url, long& response_code, std::string& result);
      bool performPost(std::string url, std::string dataStr, long& response_code, std::string& result);
      bool performMultiPost(std::string url, const curl_httppost* fields, long& response_code, std::string& result);
      response codeForError(int httpcode, std::string errors) const;
  };
  
};

#endif /* end of include guard: TWITTER_H_ABFF6A12 */
