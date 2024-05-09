cmake_minimum_required( VERSION 3.20 ) # Latest version of CMake when this file was created.

set( m_TargetName "App" )
project( "${m_TargetName}" LANGUAGES CXX )

set( HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/App.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/App_Windows.h"
)

set( SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/App.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/App_Windows.cpp"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries( "${m_TargetName}"
	PUBLIC
		Bogus::Core
		Bogus::External
)
set_target_properties( "${m_TargetName}"
    PROPERTIES
        FOLDER "Bogus"
)


# Set the startup project
set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT "${m_TargetName}" )
