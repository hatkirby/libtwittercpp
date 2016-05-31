#include "configuration.h"
#include <json.hpp>
#include <cassert>

using nlohmann::json;

namespace twitter {
  
  configuration::configuration()
  {
    _valid = false;
  }
  
  configuration::configuration(std::string data)
  {
    _valid = true;
    
    auto _data = json::parse(data);
    
    _characters_reserved_per_media = _data.at("characters_reserved_per_media");
    _dm_text_character_limit = _data.at("dm_text_character_limit");
    _max_media_per_upload = _data.at("max_media_per_upload");
    _photo_size_limit = _data.at("photo_size_limit");
    _short_url_length = _data.at("short_url_length");
    _short_https_url_length = _data.at("short_url_length_https");
    
    for (json::iterator sizedata = _data.at("photo_sizes").begin(); sizedata != _data.at("photo_sizes").end(); ++sizedata)
    {
      photosize size;
      size.height = sizedata.value().at("h");
      size.width = sizedata.value().at("w");
      if (sizedata.value().at("resize") == "fit")
      {
        size.resize = resizetype::fit;
      } else {
        size.resize = resizetype::crop;
      }
      
      _photo_sizes[sizedata.key()] = size;
    }
    
    for (auto path : _data.at("non_username_paths"))
    {
      _non_username_paths.insert(path.get<std::string>());
    }
  }
  
  size_t configuration::getCharactersReservedPerMedia() const
  {
    assert(_valid);
    
    return _characters_reserved_per_media;
  }
  
  size_t configuration::getDirectMessageCharacterLimit() const
  {
    assert(_valid);
    
    return _dm_text_character_limit;
  }
  
  size_t configuration::getMaxMediaPerUpload() const
  {
    assert(_valid);
    
    return _max_media_per_upload;
  }
  
  size_t configuration::getPhotoSizeLimit() const
  {
    assert(_valid);
    
    return _photo_size_limit;
  }
  
  std::map<std::string, configuration::photosize> configuration::getPhotoSizes() const
  {
    assert(_valid);
    
    return _photo_sizes;
  }
  
  size_t configuration::getShortUrlLength() const
  {
    assert(_valid);
    
    return _short_url_length;
  }
  
  size_t configuration::getShortHttpsUrlLength() const
  {
    assert(_valid);
    
    return _short_https_url_length;
  }
  
  std::set<std::string> configuration::getNonUsernamePaths() const
  {
    assert(_valid);
    
    return _non_username_paths;
  }
  
  configuration::operator bool() const
  {
    return _valid;
  }
  
};
