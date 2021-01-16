cmake_minimum_required(VERSION 3.13)
project(ORM_Lite CXX)

set(VERSION_MAJOR   0   CACHE STRING "Project major version number.")
set(VERSION_MINOR   0   CACHE STRING "Project minor version number.")
set(VERSION_PATCH   0   CACHE STRING "Project patch version number.")

message(STATUS "Build Documentation OPEN")
include(build_doxygen.cmake)
build_doxygen()

file(GLOB CLANG_FORMAT_BIN_NAME /usr/bin/clang-format)
if(NOT CLANG_FORMAT_BIN_NAME-NOTFOUND)
    message(STATUS "Code Formatter:  ${CLANG_FORMAT_BIN_NAME}")
    file(GLOB_RECURSE ALL_SOURCE_FILES *.h *.cc)
    add_custom_target(
            format
            COMMAND ${CLANG_FORMAT_BIN_NAME}
            -style=google
            -i
            ${ALL_SOURCE_FILES}
    )
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
enable_testing()