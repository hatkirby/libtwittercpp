cmake_minimum_required (VERSION 3.1)
project (twitter++)

include_directories(vendor/json/src)

set(LIBOAUTHCPP_BUILD_DEMOS FALSE)
add_subdirectory(vendor/liboauthcpp/build EXCLUDE_FROM_ALL)
include_directories(vendor/liboauthcpp/include)

add_subdirectory(vendor/curlcpp)
include_directories(${CURLCPP_SOURCE_DIR}/include)

add_library(twitter++ src/client.cpp src/auth.cpp src/tweet.cpp src/codes.cpp src/notification.cpp src/direct_message.cpp src/list.cpp src/user.cpp)
set_property(TARGET twitter++ PROPERTY CXX_STANDARD 11)
set_property(TARGET twitter++ PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(twitter++ oauthcpp curlcpp)
