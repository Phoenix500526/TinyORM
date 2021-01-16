cmake_minimum_required(VERSION 3.13)
project(ORM_Lite CXX)

set(VERSION_MAJOR   0   CACHE STRING "Project major version number.")
set(VERSION_MINOR   0   CACHE STRING "Project minor version number.")
set(VERSION_PATCH   0   CACHE STRING "Project patch version number.")

message(STATUS "Build Documentation OPEN")
include(build_doxygen.cmake)
build_doxygen()

