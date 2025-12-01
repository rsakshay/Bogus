cmake_minimum_required( VERSION 3.20 ) # Latest version of CMake when this file was created.

set( m_TargetName "Core" )
project( "${m_TargetName}" LANGUAGES CXX )

set( HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/Core_Arena.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/Core_Assert.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/Core_Memory.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/Core_String.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/Core_Vector.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/Core_Utility.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/Globals.h"
)

set( SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core_Arena.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core_Assert.cpp"
    #TODO(asr): Check platform and conditionally include
    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/CoreWindows_Memory.cpp"
)

add_library( "${m_TargetName}"
    STATIC
    ${HEADER_FILES}
    ${SRC_FILES}
)

add_library( "Bogus::${m_TargetName}" ALIAS "${m_TargetName}" )

target_include_directories( "${m_TargetName}"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries( "${m_TargetName}"
 PUBLIC
    Bogus::External::SMHasher
)
set_target_properties( "${m_TargetName}"
    PROPERTIES
        FOLDER "Bogus"
)


# Set the startup project
set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT "${m_TargetName}" )
