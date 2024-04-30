#make file for the base library
project(base LANGUAGES CXX VERSION 0.0.1)


file(GLOB incs ${PROJECT_SOURCE_DIR}/include/*.h)
file(GLOB srcs ${PROJECT_SOURCE_DIR}/src/*.cpp)

SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_subdirectory(${PROJECT_SOURCE_DIR}/bcrypt)

add_library(base SHARED ${incs} ${srcs})
set_target_properties(base PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(base PRIVATE ${PROJECT_SOURCE_DIR}/include ${date_INCLUDE_DIR} ${spdlog_INCLUDE_DIR} ${nlohmann_json_INCLUDE_DIR} ${SQLite_INCLUDE_DIR} ${asio_INCLUDE_DIR})
target_link_libraries(base PRIVATE bbcrypt boost::boost spdlog::spdlog fmt::fmt nlohmann_json::nlohmann_json SQLite::SQLite3 LZ4::lz4_static date::date openssl::openssl ZLIB::ZLIB)
target_link_libraries(base PUBLIC bbcrypt)