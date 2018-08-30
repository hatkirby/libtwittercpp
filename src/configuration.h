#ifndef CONFIGURATION_H_A7164D18
#define CONFIGURATION_H_A7164D18

#include <map>
#include <string>
#include <set>

namespace twitter {

  class configuration {
  public:
    enum class resizetype {
      fit,
      crop
    };

    struct photosize {
      size_t height;
      size_t width;
      resizetype resize;
    };

    explicit configuration(std::string data);

    size_t getCharactersReservedPerMedia() const
    {
      return _characters_reserved_per_media;
    }

    size_t getDirectMessageCharacterLimit() const
    {
      return _dm_text_character_limit;
    }

    size_t getMaxMediaPerUpload() const
    {
      return _max_media_per_upload;
    }

    size_t getPhotoSizeLimit() const
    {
      return _photo_size_limit;
    }

    const std::map<std::string, photosize>& getPhotoSizes() const
    {
      return _photo_sizes;
    }

    size_t getShortUrlLength() const
    {
      return _short_url_length;
    }

    size_t getShortHttpsUrlLength() const
    {
      return _short_https_url_length;
    }

    const std::set<std::string>& getNonUsernamePaths() const
    {
      return _non_username_paths;
    }

  private:

    size_t _characters_reserved_per_media;
    size_t _dm_text_character_limit;
    size_t _max_media_per_upload;
    size_t _photo_size_limit;
    std::map<std::string, photosize> _photo_sizes;
    size_t _short_url_length;
    size_t _short_https_url_length;
    std::set<std::string> _non_username_paths;
  };

};

#endif /* end of include guard: CONFIGURATION_H_A7164D18 */
