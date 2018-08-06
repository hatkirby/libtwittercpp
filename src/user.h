#ifndef USER_H_BF3AB38C
#define USER_H_BF3AB38C

#include <string>
#include <set>
#include <cassert>

namespace twitter {

  class client;

  typedef unsigned long long user_id;

  class user {
    public:

      user() {}
      user(std::string data);

      user_id getID() const
      {
        assert(_valid);

        return _id;
      }

      std::string getScreenName() const
      {
        assert(_valid);

        return _screen_name;
      }

      std::string getName() const
      {
        assert(_valid);

        return _name;
      }

      bool isProtected() const
      {
        assert(_valid);

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

      bool _valid = false;
      user_id _id;
      std::string _screen_name;
      std::string _name;
      bool _protected = false;
  };

};

#endif /* end of include guard: USER_H_BF3AB38C */
