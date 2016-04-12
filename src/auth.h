#ifndef AUTH_H_48EF85FD
#define AUTH_H_48EF85FD

#include <string>

namespace twitter {
  
  class auth {
    public:
      void setConsumerKey(std::string _arg);
      void setConsumerSecret(std::string _arg);
      void setAccessKey(std::string _arg);
      void setAccessSecret(std::string _arg);
      
      std::string getConsumerKey() const;
      std::string getConsumerSecret() const;
      std::string getAccessKey() const;
      std::string getAccessSecret() const;
      
    private:
      std::string _consumer_key;
      std::string _consumer_secret;
      std::string _access_key;
      std::string _access_secret;
  };
  
};

#endif /* end of include guard: AUTH_H_48EF85FD */
