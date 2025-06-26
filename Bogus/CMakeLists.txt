cmake_minimum_required( VERSION 3.20 ) # Latest version of CMake when this file was created.

project( "Bogus" )

set( m_BogusLibraries
    "${CMAKE_CURRENT_SOURCE_DIR}/External"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core"
    "${CMAKE_CURRENT_SOURCE_DIR}/App"
)

################################################################################

foreach( library ${m_BogusLibraries} )

    if( EXISTS "${library}" )
        add_subdirectory( "${library}" )
    endif()

endforeach()

set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" PARENT_SCOPE )

################################################################################
