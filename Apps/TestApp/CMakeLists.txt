cmake_minimum_required( VERSION 3.20 ) # Latest version of CMake when this file was created.

set( m_TargetName "TestApp" )
string( REGEX MATCH "[^/]*$" m_BuildDir "${CMAKE_BINARY_DIR}" )

# Use folders in IDEs.
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################
# Supported build configurations.
set( m_Configurations
    "Debug"
    "Release"
)
set( BuildType "Debug" CACHE STRING "The type of build to generate (${m_Configurations})." )
set_property( CACHE BuildType PROPERTY STRINGS ${m_Configurations} )

if( NOT BuildType IN_LIST m_Configurations )
    message( FATAL_ERROR "Invalid BuildType [${BuildType}]. Valid options are: ${m_Configurations}" )
endif()


# THIS ONE LINE defines the authoritative version number for the Indus app (and its settings files).
if(WIN32)
project( "${m_TargetName} ${m_BuildDir}"
         LANGUAGES C CXX RC )
else()
project( "${m_TargetName} ${m_BuildDir}"
         LANGUAGES C CXX )
endif()



message( STATUS "Build Type [${BuildType}]." )

add_compile_definitions( "ASR_BUILD_TYPE=\"${BuildType}\"" )
if( BuildType STREQUAL "Debug" )
    add_compile_definitions( "ASR_DEBUG" )
else()
    add_compile_definitions( "ASR_RELEASE" )
endif()

set( HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TestApp_App.h"
)

set( SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TestApp_App.cpp"
)

add_executable( "${m_TargetName}" WIN32
    ${HEADER_FILES}
    ${SRC_FILES}
)

target_include_directories( "${m_TargetName}"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries( "${m_TargetName}"
)

target_link_libraries( "${m_TargetName}"
 PRIVATE
  Bogus::App
  Bogus::Core
  Bogus::External
        Bogus::Renderer
)

# Set the startup project
set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT "${m_TargetName}" )
