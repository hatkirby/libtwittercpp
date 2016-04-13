#ifndef CODES_H_05838D39
#define CODES_H_05838D39

#include <ostream>

namespace twitter {
  
  enum class response {
    ok,
    curl_error,
    bad_auth,
    limited,
    server_error,
    server_unavailable,
    server_overloaded,
    server_timeout,
    suspended,
    bad_token,
    duplicate_status,
    suspected_spam,
    write_restricted,
    bad_length,
    unknown_error,
    invalid_media
  };
  
};

std::ostream& operator<<(std::ostream& os, twitter::response r);

#endif /* end of include guard: CODES_H_05838D39 */
