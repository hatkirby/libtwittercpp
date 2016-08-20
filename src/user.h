#ifndef USER_H_BF3AB38C
#define USER_H_BF3AB38C

#include <string>
#include <set>

namespace twitter {
  
  class client;
  
  typedef unsigned long long user_id;
  
  class user {
    public:
      
      user(const client& tclient, std::string data);
      
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
      
      bool operator==(const user& other) const
      {
        return _id == other._id;
      }
      
      bool operator!=(const user& other) const
      {
        return _id != other._id;
      }
      
      std::set<user_id> getFriends() const;
      std::set<user_id> getFollowers() const;
      void follow() const;
      void unfollow() const;
      
    private:
      
      const client& _client;
      user_id _id;
      std::string _screen_name;
      std::string _name;
  };
  
};

#endif /* end of include guard: USER_H_BF3AB38C */
