#include "codes.h"
#include <sstream>

namespace twitter {
  
  const char* invalid_response::WHAT_TEXT = "Invalid response data received from Twitter";
  const char* connection_error::WHAT_TEXT = "Error connecting to Twitter";
  
  std::string unknown_error::generateMessage(int response_code)
  {
    std::ostringstream msgbuilder;
    msgbuilder << "Unknown error (HTTP " << response_code << ")";
    
    return msgbuilder.str();
  }

};
