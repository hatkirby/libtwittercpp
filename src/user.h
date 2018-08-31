#ifndef USER_H_BF3AB38C
#define USER_H_BF3AB38C

#include <string>

namespace twitter {

  typedef unsigned long long user_id;

  class user {
  public:

    user(std::string data);

    user_id getID() const
    {
      return _id;
    }

    std::string getScreenName() const
    {
      return _screen_name;
    }

    std::string getName() const
    {
      return _name;
    }

    bool isProtected() const
    {
      return _protected;
    }

    bool operator==(const user& other) const
    {
      return _id == other._id;
    }

    bool operator!=(const user& other) const
    {
      return _id != other._id;
    }

  private:

    user_id _id;
    std::string _screen_name;
    std::string _name;
    bool _protected = false;
  };

};

#endif /* end of include guard: USER_H_BF3AB38C */
