#
# Scarab's unit tests (doctest) - doc/MASTER_PLAN.md item R2. Built only with
# -DSCARAB_BUILD_TESTS=ON; run ./build/scarab_tests (build/Release/scarab_tests.exe on Windows).
# Deliberately not registered with ctest: the fetched Lua project registers its own failing
# "lua-testsuite" there, so ctest would run (and fail on) a suite that isn't Scarab's.
#
# They test what needs no window: the Lua-agnostic pools (src/engine/), LuaEngineUtil's
# argument readers on a bare lua_State, and pure static helpers. Whole Lua functions are
# covered by the headless smoke tests (scripts/*_smoke.py).
#

include(FetchContent)

# The same doctest release sunlight's own tests pin.
FetchContent_Declare(doctest
                     GIT_REPOSITORY https://github.com/doctest/doctest.git
                     GIT_TAG v2.4.11)
FetchContent_MakeAvailable(doctest)

file(GLOB SCARAB_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(scarab_tests ${SCARAB_TEST_SOURCES})
target_link_libraries(scarab_tests PRIVATE scarab_core doctest::doctest)

# Next to scarab (same runtime libraries, copied there by scarab_copy_binaries), with the same
# relocatable rpath - see the rpath block in the root CMakeLists.txt.
set_target_properties(scarab_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
add_dependencies(scarab_tests scarab_copy_binaries)

if(APPLE)
  set_target_properties(scarab_tests PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set_target_properties(scarab_tests PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "$ORIGIN")
endif()
