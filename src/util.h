#ifndef UTIL_H_440DEAA0
#define UTIL_H_440DEAA0

#include <string>
#include <sstream>
#include <memory>

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

};

#endif /* end of include guard: UTIL_H_440DEAA0 */
