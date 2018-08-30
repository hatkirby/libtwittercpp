#include "configuration.h"
#include <json.hpp>
#include <cassert>
#include "codes.h"

namespace twitter {

  configuration::configuration(std::string data) try
  {
    auto json_data = nlohmann::json::parse(data);

    _characters_reserved_per_media = json_data["characters_reserved_per_media"].get<size_t>();
    _dm_text_character_limit = json_data["dm_text_character_limit"].get<size_t>();
    _max_media_per_upload = json_data["max_media_per_upload"].get<size_t>();
    _photo_size_limit = json_data["photo_size_limit"].get<size_t>();
    _short_url_length = json_data["short_url_length"].get<size_t>();
    _short_https_url_length = json_data["short_url_length_https"].get<size_t>();

    for (auto sizedata = std::begin(json_data["photo_sizes"]); sizedata != std::end(json_data["photo_sizes"]); ++sizedata)
    {
      photosize size;
      size.height = sizedata.value()["h"].get<size_t>();
      size.width = sizedata.value()["w"].get<size_t>();
      if (sizedata.value()["resize"].get<std::string>() == "fit")
      {
        size.resize = resizetype::fit;
      } else {
        size.resize = resizetype::crop;
      }

      _photo_sizes[sizedata.key()] = size;
    }

    for (auto path : json_data["non_username_paths"])
    {
      _non_username_paths.insert(path.get<std::string>());
    }
  } catch (const std::invalid_argument& error)
  {
    std::throw_with_nested(malformed_object("configuration", data));
  } catch (const std::domain_error& error)
  {
    std::throw_with_nested(malformed_object("configuration", data));
  }

};
