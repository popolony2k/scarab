cmake_minimum_required (VERSION 2.6)

project (GameEngine)

# Add all include files
include_directories("src/include")

# Add all source files
file( GLOB_RECURSE APPLICATION_SOURCES "src/source" *.cpp )
file( GLOB APPLICATION_HEADERS src/include/*.h )

file(GLOB_RECURSE REMOVE_CMAKE "build/*")
list(REMOVE_ITEM APPLICATION_SOURCES ${REMOVE_CMAKE})

# Debug information (comment to turn off)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g2")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(GameEngine ${APPLICATION_SOURCES} ${APPLICATION_HEADERS})

target_link_libraries(GameEngine pthread raylib tmx dl z xml2 m)
