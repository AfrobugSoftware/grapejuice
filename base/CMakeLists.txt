#make file for the base library
project(base LANGUAGES CXX VERSION 0.0.1)


file(GLOB incs ${PROJECT_SOURCE_DIR}/include/*.h)
file(GLOB srcs ${PROJECT_SOURCE_DIR}/src/*.cpp)

SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_subdirectory(${PROJECT_SOURCE_DIR}/bcrypt)

add_library(base SHARED ${incs} ${srcs})
set_target_properties(base PROPERTIES POSITION_INDEPENDENT_CODE ON)

message(STATUS ${CONAN_INCLUDE_DIRS_DATE})

target_include_directories(base PUBLIC ${PROJECT_SOURCE_DIR}/include ${CONAN_INCLUDE_DIRS_BOOST} ${CONAN_INCLUDE_DIRS_DATE})
target_link_directories(base PRIVATE ${CONAN_LIB_DIRS_BOOST})
target_link_libraries(base PRIVATE bbcrypt boost_serialization boost_url boost_iostreams boost_system ${CONAN_LIBS_OPENSSL} ${CONAN_LIBS_ZLIB} ${CONAN_LIBS_FMT} ${CONAN_LIBS_SPDLOG} ${CONAN_LIBS_SQLITE3} ${CONAN_LIBS_DATE})
target_link_libraries(base PUBLIC bbcrypt)