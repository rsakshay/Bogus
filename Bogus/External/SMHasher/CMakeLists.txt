cmake_minimum_required( VERSION 3.20 ) # Latest version of CMake when this file was created.

set( m_TargetName "SMHasher" )
project( "${m_TargetName}" LANGUAGES CXX )

set( HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/MurmurHash3.h"
)

set( SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MurmurHash3.cpp"
)

add_library( "${m_TargetName}"
    STATIC
    ${HEADER_FILES}
    ${SRC_FILES}
)

add_library( "Bogus::External::${m_TargetName}" ALIAS "${m_TargetName}" )

target_include_directories( "${m_TargetName}"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries( "${m_TargetName}"
)
set_target_properties( "${m_TargetName}"
    PROPERTIES
    FOLDER "Bogus_External"
)


# Set the startup project
set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT "${m_TargetName}" )
