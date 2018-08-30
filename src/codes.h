#ifndef CODES_H_05838D39
#define CODES_H_05838D39

#include <stdexcept>
#include <string>

namespace twitter {

  class twitter_error : public std::runtime_error {
  public:

    using std::runtime_error::runtime_error;
  };

  class bad_auth : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class invalid_response : public twitter_error {
  public:

    static const char* WHAT_TEXT;

    explicit invalid_response(std::string response) noexcept
      : twitter_error(WHAT_TEXT), _response(std::move(response))
    {
    }

    const std::string& getResponse() const noexcept
    {
      return _response;
    }

  private:

    std::string _response;
  };

  class account_suspended : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class rate_limit_exceeded : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class update_limit_exceeded : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class bad_token : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class server_overloaded : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class server_error : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class bad_length : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class duplicate_status : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class suspected_spam : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class write_restricted : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class invalid_media : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class server_unavailable : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class server_timeout : public twitter_error {
  public:

    using twitter_error::twitter_error;
  };

  class unknown_error : public twitter_error {
  public:

    unknown_error(int response_code, std::string response_data)
      : twitter_error(generateMessage(response_code)),
        _response_code(response_code),
        _response_data(std::move(response_data))
    {
    }

    int getResponseCode() const noexcept
    {
      return _response_code;
    }

    const std::string& getResponse() const noexcept
    {
      return _response_data;
    }

  private:

    static std::string generateMessage(int response_code);

    int _response_code;
    std::string _response_data;
  };

  class connection_error : public twitter_error {
  public:

    static const char* WHAT_TEXT;

    connection_error() noexcept : twitter_error(WHAT_TEXT)
    {
    }
  };

  class invalid_member : public std::domain_error {
  public:

    using std::domain_error::domain_error;
  };

  class malformed_object : public invalid_response {
  public:

    malformed_object(std::string type, std::string data) noexcept
      : invalid_response(std::move(data)), _type(std::move(type))
    {
    }

    const std::string& getType() const noexcept
    {
      return _type;
    }

  private:

    std::string _type;
  };

};

#endif /* end of include guard: CODES_H_05838D39 */
