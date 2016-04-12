#include "twitter.h"

namespace twitter {
  
  void auth::setConsumerKey(std::string _arg)
  {
    _consumer_key = _arg;
  }
  
  void auth::setConsumerSecret(std::string _arg)
  {
    _consumer_secret = _arg;
  }
  
  void auth::setAccessKey(std::string _arg)
  {
    _access_key = _arg;
  }
  
  void auth::setAccessSecret(std::string _arg)
  {
    _access_secret = _arg;
  }
  
  std::string auth::getConsumerKey() const
  {
    return _consumer_key;
  }
  
  std::string auth::getConsumerSecret() const
  {
    return _consumer_secret;
  }
  
  std::string auth::getAccessKey() const
  {
    return _access_key;
  }
  
  std::string auth::getAccessSecret() const
  {
    return _access_secret;
  }
    
};
