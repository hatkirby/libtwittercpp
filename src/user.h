#ifndef USER_H_BF3AB38C
#define USER_H_BF3AB38C

#include <string>

namespace twitter {
  
  typedef unsigned long long user_id;
  
  class user {
    public:
      user();
      user(std::string data);
      
      user_id getID() const;
      std::string getScreenName() const;
      std::string getName() const;
      
      operator bool() const;
      bool operator==(const user& other) const;
      
    private:
      bool _valid = false;
      user_id _id;
      std::string _screen_name;
      std::string _name;
  };
  
};

#endif /* end of include guard: USER_H_BF3AB38C */
