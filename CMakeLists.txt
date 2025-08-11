cmake_minimum_required(VERSION 3.16)
project(API-Wrapper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_library(api_wrapper STATIC
    src/http_client.cpp
)
target_include_directories(api_wrapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
target_link_libraries(api_wrapper PUBLIC CURL::libcurl)

add_executable(main
    src/main.cpp
)
target_link_libraries(main PRIVATE api_wrapper)
