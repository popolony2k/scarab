#
# CMake file for building Scarab (the engine) and the games built on it.
# Copyright (c) since 2021 by PopolonY2k
#

cmake_minimum_required(VERSION 3.24)

project(pog-games-distribution)

include(CMakeDependentOption)
include(GNUInstallDirs)
include(FetchContent)
include(ExternalProject)

option(SCARAB_DEBUG_SYMBOLS_UNIX "Build Scarab with debug symbols on Unixes (Mac and Linux)" ON)
option(SCARAB_USE_OFFICIAL_LUA_FTP "Build Scarab with LUA provided by official FTP (Windows VS Studio unsupported)" OFF)

####################################################################
# sunlight
####################################################################
FetchContent_Declare(sunlight
                     GIT_REPOSITORY https://github.com/popolony2k/sunlight.git
                     GIT_TAG v0.15.0)
FetchContent_MakeAvailable(sunlight)

####################################################################
# nlohmann/json
####################################################################
FetchContent_Declare(nlohmann_json
                     GIT_REPOSITORY https://github.com/nlohmann/json.git
                     GIT_TAG master)
FetchContent_MakeAvailable(nlohmann_json)

if(NOT SCARAB_USE_OFFICIAL_LUA_FTP)
####################################################################
# walterschell/Lua (CMake based Lua)
####################################################################
  FetchContent_Declare(lua_library
                      GIT_REPOSITORY https://github.com/walterschell/Lua.git
                      GIT_TAG master)
  FetchContent_MakeAvailable(lua_library)

  set(LUA_LIBRARIES lua_static)
else()
  #
  # Lua integration based on official FTP distribution (Start)
  #
  # ####################################################################
  # # Lua (MakeFile based project - Only UNIX-Like)
  # ####################################################################
  find_program(MAKE_EXE NAMES gmake nmake make)
  ExternalProject_Add(lua_library_download
                      URL http://www.lua.org/ftp/lua-5.4.6.tar.gz
                      UPDATE_COMMAND ""
                      CONFIGURE_COMMAND ""
                      INSTALL_COMMAND ""
                      SOURCE_DIR ${lua_engine_SOURCE_DIR}
                      BUILD_IN_SOURCE 1
                      BUILD_COMMAND ${MAKE_EXE}
                      BUILD_BYPRODUCTS
                      DOWNLOAD_EXTRACT_TIMESTAMP false
                      PREFIX ${CMAKE_CURRENT_BINARY_DIR})
  ExternalProject_Get_Property(lua_library_download SOURCE_DIR BINARY_DIR)

  # Adjust liblua name based on OS
  set(LUA_ENGINE_LIB_PATH ${SOURCE_DIR}/src/)
  set(LUA_ENGINE_LIB_NAME ${LUA_ENGINE_LIB_PATH}/${CMAKE_STATIC_LIBRARY_PREFIX}lua${CMAKE_STATIC_LIBRARY_SUFFIX})

  file (MAKE_DIRECTORY ${LUA_ENGINE_LIB_PATH})
  file (TOUCH ${LUA_ENGINE_LIB_NAME})

  # Creating diretory and file is needed because linker will fail at build start
  add_library(lua_library SHARED IMPORTED)
  set_target_properties(lua_library PROPERTIES IMPORTED_LOCATION ${LUA_ENGINE_LIB_NAME})
  set(LUA_LIBRARIES lua_library)

  # Add lua include directory
  include_directories(${LUA_ENGINE_LIB_PATH})

  #
  # Lua integration based on official FTP distribution (End)
  #

  #
  # Lua integration based on FindPackage (Start)
  #
  find_package(Lua REQUIRED)
  include_directories(${LUA_INCLUDE_DIR})

  #
  # Lua integration based on FindPackage (End)
  #
endif()

# Add all include files
include_directories("src")
include_directories("src/lua")
include_directories("src/helper")
include_directories("src/host")
include_directories("src/engine")

# Add all source files
#
# GLOB_RECURSE's pattern must be a single "src/*.cpp" expression, not two
# separate arguments ("src" *.cpp) - the latter globs *.cpp recursively from
# CMAKE_CURRENT_SOURCE_DIR itself (unscoped), "src" being matched as its own,
# unrelated glob expression. Harmless while this ran from
# games/caravellius/CMakeLists.txt (nothing else under that directory had
# stray .cpp files), but moving src/ - and this glob - to the repo root
# started sweeping up FetchContent'd dependencies' own .cpp files from
# build/_deps/ too, breaking the build with unrelated compile errors.
#
file( GLOB_RECURSE APPLICATION_SOURCES "src/*.cpp" )
file( GLOB APPLICATION_HEADERS "src/*.h" )

#
# C++17 is required project-wide (std::filesystem, and some raylib defines on APPLE
# need it too) - must not be gated to a single platform, or other generators/compilers
# (e.g. MSVC on Windows, GCC's own pre-C++17 default) silently fall back to an older
# standard and std::filesystem fails to compile.
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX)
    #
    # Debug information
    #
    if(SCARAB_DEBUG_SYMBOLS_UNIX)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g2")
    endif()

    if(NOT APPLE)
      #
      # PThread support
      #
      include(FindThreads)

      if(THREADS_FOUND AND NOT TARGET Threads::Threads)
        add_library(Threads::Threads INTERFACE IMPORTED)

        if(THREADS_HAVE_PTHREAD_ARG)
            set_property(TARGET Threads::Threads PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
        endif()

        if(CMAKE_THREAD_LIBS_INIT)
            set_property(TARGET Threads::Threads PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
        endif()
      endif()
    endif()
endif()

add_executable(scarab ${APPLICATION_SOURCES} ${APPLICATION_HEADERS})

if(SCARAB_USE_OFFICIAL_LUA_FTP)
  add_dependencies(scarab lua_library_download)
endif()

target_link_libraries(scarab ${CMAKE_THREAD_LIBS_INIT} sunlight nlohmann_json ${LUA_LIBRARIES})

add_custom_target(scarab_copy_binaries
COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LibXml2> $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:raylib> $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:tmx> $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sunlight> $<TARGET_FILE_DIR:scarab>
COMMENT "Copying Scarab's own runtime dependency binaries" VERBATIM)

if(WIN32 AND DEFINED Iconv_LIBRARY)
  # libxml2 (Iconv) and tmx (zlib) both link these vcpkg-provided libraries
  # dynamically on Windows. Their own IMPORTED targets (Iconv::Iconv,
  # ZLIB::ZLIB) are created by find_package() calls inside sibling
  # FetchContent subdirectories (libxml2-src/tmx-src' own CMakeLists.txt) via
  # CMake's bundled Find modules, which create those targets WITHOUT the
  # GLOBAL keyword - so unlike raylib/tmx/sunlight above (targets this same
  # CMakeLists.txt declares directly), $<TARGET_FILE:Iconv::Iconv> would be
  # invisible here and silently skip an if(TARGET ...) guard rather than
  # error. Iconv_LIBRARY/ZLIB_LIBRARY_* are plain CACHE variables instead, so
  # they *are* visible here; derive the runtime DLLs from vcpkg's lib/ ->
  # bin/ layout next to them, selecting release/debug per the active config
  # the same way $<TARGET_FILE:...> does for the CMake-target deps above.
  get_filename_component(_iconv_lib_dir "${Iconv_LIBRARY}" DIRECTORY)
  get_filename_component(_vcpkg_installed_prefix "${_iconv_lib_dir}" DIRECTORY)

  find_file(ICONV_RUNTIME_DLL_RELEASE NAMES iconv-2.dll libiconv-2.dll iconv.dll
            PATHS "${_vcpkg_installed_prefix}/bin" NO_DEFAULT_PATH)
  find_file(ICONV_RUNTIME_DLL_DEBUG NAMES iconv-2.dll libiconv-2.dll iconv.dll
            PATHS "${_vcpkg_installed_prefix}/debug/bin" NO_DEFAULT_PATH)
  find_file(ZLIB_RUNTIME_DLL_RELEASE NAMES zlib1.dll zlib.dll
            PATHS "${_vcpkg_installed_prefix}/bin" NO_DEFAULT_PATH)

  if(NOT ICONV_RUNTIME_DLL_DEBUG)
    set(ICONV_RUNTIME_DLL_DEBUG "${ICONV_RUNTIME_DLL_RELEASE}")
  endif()

  if(ICONV_RUNTIME_DLL_RELEASE)
    add_custom_command(TARGET scarab_copy_binaries POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<IF:$<CONFIG:Debug>,${ICONV_RUNTIME_DLL_DEBUG},${ICONV_RUNTIME_DLL_RELEASE}>"
              $<TARGET_FILE_DIR:scarab>
      COMMENT "Copying Iconv runtime DLL (Windows/vcpkg dependency of libxml2)" VERBATIM)
  endif()
  if(ZLIB_RUNTIME_DLL_RELEASE)
    # Unlike Iconv, vcpkg's zlib debug build renames the DLL on disk to
    # zlibd1.dll during packaging but doesn't update the import-library
    # metadata baked into anything that links against it - so tmx.dll always
    # imports a DLL literally named "zlib1.dll" (confirmed via dumpbin),
    # even in a Debug build linking the debug zlib.lib variant. Always copy
    # the release-named DLL under that exact name, for both configs.
    add_custom_command(TARGET scarab_copy_binaries POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${ZLIB_RUNTIME_DLL_RELEASE}" $<TARGET_FILE_DIR:scarab>
      COMMENT "Copying zlib runtime DLL (Windows/vcpkg dependency of tmx)" VERBATIM)
  endif()
endif()

if(WIN32)
  # A Debug build's scarab.exe links MSVCP140D.dll/VCRUNTIME140D.dll/
  # VCRUNTIME140_1D.dll/ucrtbased.dll (the debug CRT). Those aren't part of
  # Windows itself (unlike the release CRT, already present system-wide) and
  # only live inside the Visual Studio install tree. The full Visual Studio
  # IDE adds that folder to PATH automatically when debugging; VSCode's
  # cppvsdbg (.vscode/launch.json's "(Windows) Launch" config) does not, so
  # a Debug build launched that way fails with STATUS_DLL_NOT_FOUND unless
  # these are copied next to the executable like every other runtime DLL
  # here. InstallRequiredSystemLibraries resolves the exact redist DLLs for
  # whatever MSVC toolset is active, so this stays correct across VS
  # updates rather than hardcoding a version-specific path.
  set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
  set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
  include(InstallRequiredSystemLibraries)
  if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    add_custom_command(TARGET scarab_copy_binaries POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} $<TARGET_FILE_DIR:scarab>
      COMMENT "Copying MSVC/UCRT runtime redistributable DLLs (Debug-CRT dependency of cppvsdbg launches)" VERBATIM)
  endif()
endif()

add_dependencies(scarab scarab_copy_binaries)

###################################################################
# Setup games built on Scarab
###################################################################

add_subdirectory( games/caravellius )

# Installation
# INSTALL(
#   TARGETS scarab sunlight
#   RUNTIME_DEPENDENCIES
#   RUNTIME DESTINATION "${CMAKE_INSTALL_PATH}/bin"
# )
