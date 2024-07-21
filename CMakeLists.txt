cmake_minimum_required(VERSION 3.20)
project(grapejuice VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_UNITY_BUILD TRUE)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(lz4 REQUIRED)
find_package(date REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Crow REQUIRED)

file(GLOB incs ${PROJECT_SOURCE_DIR}/include/*.h)
file(GLOB srcs ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_subdirectory(${PROJECT_SOURCE_DIR}/base)
#add_compile_definitions(-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING )
add_executable(grapejuice ${incs} ${srcs}) 

if(CMAKE_SYSTEM MATCHES Linux)
	target_compile_options(grapejuice PUBLIC -fpremissive -fcoroutines)
elseif(CMAKE_SYSTEM MATCHES Windows)
	target_compile_options(grapejuice PUBLIC /bigobj /Zc:__cplusplus /JMC-)
endif()


target_precompile_headers(grapejuice PUBLIC ${incs})
target_include_directories(grapejuice PUBLIC ${Crow_INCLUDE_DIR} ${spdlog_INCLUDE_DIR} ${nlohmann_json_INCLUDE_DIR} ${SQLite_INCLUDE_DIR} ${date_INCLUDE_DIR} ${asio_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/base/include ${PROJECT_SOURCE_DIR}/include)
target_link_directories(grapejuice PUBLIC ${BASE_LIB_DIR})
target_link_libraries(grapejuice PUBLIC base boost::boost spdlog::spdlog fmt::fmt nlohmann_json::nlohmann_json SQLite::SQLite3 LZ4::lz4_static date::date openssl::openssl ZLIB::ZLIB Crow::Crow)

