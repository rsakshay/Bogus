# Minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.0)

# Name of the project
project(Bogus)

# Add all the source files needed to build the executable
add_executable(
  Bogus WIN32
  src/main.cpp
)

# Set Bogus as the startup project
set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT "Bogus" )