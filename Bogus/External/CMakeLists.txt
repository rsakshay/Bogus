cmake_minimum_required( VERSION 3.20 ) # Latest version of CMake when this file was created.

project( "External" )

set( m_BogusExternalLibraries
    "${CMAKE_CURRENT_SOURCE_DIR}/SMHasher"
)

################################################################################

foreach( library ${m_BogusExternalLibraries} )

    if( EXISTS "${library}" )
        add_subdirectory( "${library}" )
    endif()

endforeach()

set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" PARENT_SCOPE )

################################################################################
