#ifndef UTIL_H_440DEAA0
#define UTIL_H_440DEAA0

#include <string>
#include <sstream>

namespace twitter {
  
  template <class InputIterator>
  std::string implode(InputIterator first, InputIterator last, std::string delimiter)
  {
    std::stringstream result;
    
    for (InputIterator it = first; it != last; it++)
    {
      if (it != first)
      {
        result << delimiter;
      }
      
      result << *it;
    }
    
    return result.str();
  }
  
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
      return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
  
};

#endif /* end of include guard: UTIL_H_440DEAA0 */
