#include "codes.h"

std::ostream& operator<<(std::ostream& os, twitter::response r)
{
  switch (r)
  {
    case twitter::response::ok: return os << "OK";
    case twitter::response::curl_error: return os << "Curl Error";
    case twitter::response::bad_auth: return os << "Bad Auth";
    case twitter::response::limited: return os << "Rate Limit Exceeded";
    case twitter::response::server_error: return os << "Twitter Server Error";
    case twitter::response::server_unavailable: return os << "Twitter Is Down";
    case twitter::response::server_overloaded: return os << "Twitter Is Over Capacity";
    case twitter::response::server_timeout: return os << "Twitter Connection Timed Out";
    case twitter::response::suspended: return os << "Authenticated User Is Suspended";
    case twitter::response::bad_token: return os << "Invalid Or Expired Access Token";
    case twitter::response::duplicate_status: return os << "Duplicate Status";
    case twitter::response::suspected_spam: return os << "Request Looks Automated";
    case twitter::response::write_restricted: return os << "Cannot Perform Write";
    case twitter::response::bad_length: return os << "Message Body Too Long";
    case twitter::response::unknown_error: return os << "Unknown Error";
    case twitter::response::invalid_media: return os << "Invalid Media";
  }
}
