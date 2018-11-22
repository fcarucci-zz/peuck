cmake_minimum_required(VERSION 3.4.1)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Werror -Wthread-safety" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D _LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS -O3 -fPIC" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections" )

set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-s" )

include_directories( ../../include )

message( STATUS "Building peuck_static to ${CMAKE_CURRENT_BINARY_DIR}/build" )
add_library( peuck_static

             STATIC

             Peuck.cpp

             # Add new source files here
             )

set_target_properties( peuck_static PROPERTIES
     LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

add_library( peuck

             SHARED

             Peuck.cpp)     # Looks like I need to specify at least one file to build,
                            # the result .so size suggests it's including the rest of the
                            # source files from the library
                            # also, removing the target_link to the static library
                            # fails the build


 target_link_libraries( peuck

                        peuck_static
                        android
                        log )
