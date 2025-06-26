cmake_minimum_required(VERSION 3.15)
project(ProjectBogus LANGUAGES CXX)

# Set standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_BASEAPP "Build BaseApp application" OFF)
option(BUILD_TESTAPP "Build TestApp application" OFF)

# Add subdirectories
add_subdirectory(Bogus)
if(BUILD_BASEAPP)
    add_subdirectory(Apps/BaseApp)
endif()

if(BUILD_TESTAPP)
    add_subdirectory(Apps/TestApp)
endif()