#
# CMake file for building Scarab, a thin, game-agnostic 2D engine host.
# Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
#

cmake_minimum_required(VERSION 3.24)

#
# SCARAB_SUNLIGHT_VERSION is the one place the pinned sunlight release lives -
# both the FetchContent GIT_TAG below and the --version/-v CLI output (see
# main.cpp) read it from here, so bumping a sunlight dependency never means
# updating the tag in two places that could drift out of sync.
#
set(SCARAB_SUNLIGHT_VERSION "0.19.0")

project(scarab VERSION 0.1.8)

include(CMakeDependentOption)
include(GNUInstallDirs)
include(FetchContent)
include(ExternalProject)

option(SCARAB_DEBUG_SYMBOLS_UNIX "Build Scarab with debug symbols on Unixes (Mac and Linux)" ON)
option(SCARAB_USE_OFFICIAL_LUA_FTP "Build Scarab with LUA provided by official FTP (Windows VS Studio unsupported)" OFF)

#
# SCARAB_CONTENT_KEY: a hex-encoded 32-byte (64 hex characters) shared
# secret, compiled into this build, used by crypto_encrypt_data/
# crypto_decrypt_data (LuaCryptoApi) to protect a game's own .zip
# bundle content - see the README's own "Content encryption" section
# and tools/README.md for the full design/workflow. Deliberately
# left empty by default: crypto_encrypt_data/crypto_decrypt_data fail
# cleanly (not silently, and not with some fixed public placeholder
# key everyone would share) if no key was configured at build time.
# Every game that wants this feature sets it's own, kept private and
# NEVER committed to a public repo - pass it at configure time, e.g.
# `cmake -B build -S . -DSCARAB_CONTENT_KEY=<64 hex chars>`, typically
# from a CI secret rather than typed/stored anywhere in source control.
#
set(SCARAB_CONTENT_KEY "" CACHE STRING "Hex-encoded 32-byte shared secret for content encryption - see README.md's own \"Content encryption\" section")

#
# SCARAB_LIBSODIUM_SHARED: build libsodium as a shared/dynamic library
# (libsodium.dylib/.so/.dll) instead of the default static one. Static
# is the simpler deployment story (baked straight into scarab, nothing
# separate to ship or protect) and is what's actually been verified
# live - this option exists for whoever specifically wants the
# dynamic-linking tradeoffs instead
# (e.g. sharing one libsodium build across several binaries).
#
option(SCARAB_LIBSODIUM_SHARED "Build libsodium as a shared/dynamic library instead of static" OFF)

####################################################################
# sunlight
####################################################################
FetchContent_Declare(sunlight
                     GIT_REPOSITORY https://github.com/popolony2k/sunlight.git
                     GIT_TAG v${SCARAB_SUNLIGHT_VERSION})
FetchContent_MakeAvailable(sunlight)

####################################################################
# nlohmann/json
####################################################################
FetchContent_Declare(nlohmann_json
                     GIT_REPOSITORY https://github.com/nlohmann/json.git
                     GIT_TAG master)
FetchContent_MakeAvailable(nlohmann_json)

####################################################################
# CLI11 (command line parsing - Phase 12, archive-distribution:
# main.cpp's entry argument grew a real --entry/-e option once a bare
# argv[1]/argv[2] scan wasn't enough to express "override which project
# file the entry point resolves to." No C++ standard library facility
# does this (there's no <cliparse>; POSIX getopt isn't available on
# Windows/MSVC without extra tooling, ruling it out given this project's
# own Windows/vcpkg support). CLI11 is header-only, MIT licensed, and
# needs nothing beyond what's already fetched for every other dependency
# here.)
####################################################################
FetchContent_Declare(cli11
                     GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
                     GIT_TAG v2.4.2)
FetchContent_MakeAvailable(cli11)

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

####################################################################
# libsodium, via robinlinden/libsodium-cmake (content encryption - see
# README.md's own "Content encryption" section). libsodium's own native build
# system is Autotools, not CMake, and this project deliberately never
# mixes in a non-CMake build tool (no ExternalProject_Add shelling out
# to `./configure && make`, no vcpkg-only-on-Windows asymmetry) - this
# wrapper is a real, purpose-built CMakeLists.txt around the actual,
# unmodified official libsodium source (a git submodule, not a fork),
# the exact same shape as walterschell/Lua just above solving the
# identical problem for official (also Makefile-based) Lua. Verified
# directly (not assumed) before pinning: builds statically by default
# (BUILD_SHARED_LIBS is unset above, so it stays static - the simplest
# deployment story for a key-embedding library, no separate shared
# library to add to scarab_copy_binaries), and a real functional
# encrypt/decrypt roundtrip was confirmed live on macOS Intel before
# this was ever added here. Pinned to it's current HEAD at the time of
# adding, since the wrapper repo has no tagged releases.
####################################################################
FetchContent_Declare(sodium
                     GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
                     GIT_TAG 9b2848dfc1b917a9410f0de9d81059b26cbfaa8d)
set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)

# libsodium-cmake respects the standard BUILD_SHARED_LIBS variable (no
# custom toggle of it's own) - saved/restored around just this one
# FetchContent_MakeAvailable() call rather than set directly, since
# BUILD_SHARED_LIBS is a genuinely global CMake variable that would
# otherwise leak into every other add_library() call project-wide,
# including ones in dependencies fetched later than this block.
#
# Must be set EXPLICITLY either way (ON or OFF), not left alone when
# SCARAB_LIBSODIUM_SHARED is OFF - raylib's own CMakeLists.txt (fetched
# earlier, via sunlight above) force-caches BUILD_SHARED_LIBS=ON to
# build itself shared, and CACHE variables persist for the rest of this
# same configure pass. A first attempt here that only ever set it ON
# (and otherwise left BUILD_SHARED_LIBS untouched) silently inherited
# raylib's already-cached ON instead of defaulting to static as
# intended - confirmed live: scarab failed to even start
# ("dyld: Library not loaded: @rpath/libsodium.dylib") despite
# SCARAB_LIBSODIUM_SHARED defaulting OFF, until this was set explicitly
# in both directions.
set(__scarab_saved_build_shared_libs ${BUILD_SHARED_LIBS})
if(SCARAB_LIBSODIUM_SHARED)
  set(BUILD_SHARED_LIBS ON)
else()
  set(BUILD_SHARED_LIBS OFF)
endif()

FetchContent_MakeAvailable(sodium)

set(BUILD_SHARED_LIBS ${__scarab_saved_build_shared_libs})
unset(__scarab_saved_build_shared_libs)

####################################################################
# miniz (content encryption's own packaging tool - see tools/README.md).
# Unlike libsodium,
# miniz ships it's own real, official CMakeLists.txt (verified
# directly before pinning) - a plain FetchContent_Declare/
# MakeAvailable is enough, no wrapper needed. Always kept static here
# (unlike sodium, no SCARAB_MINIZ_SHARED toggle) - miniz is only ever
# used internally by the packaging tool itself, never something a
# consuming game links against or has reason to care about, so there's
# no case for exposing a build-mode choice for it. Same
# BUILD_SHARED_LIBS save/restore pattern as sodium above, for the same
# reason (it also just respects the standard variable, no option of
# it's own, and would otherwise inherit raylib's already-cached ON).
####################################################################
FetchContent_Declare(miniz
                     GIT_REPOSITORY https://github.com/richgel999/miniz.git
                     GIT_TAG 3.1.2)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(__scarab_saved_build_shared_libs ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(miniz)

set(BUILD_SHARED_LIBS ${__scarab_saved_build_shared_libs})
unset(__scarab_saved_build_shared_libs)

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

target_link_libraries(scarab ${CMAKE_THREAD_LIBS_INIT} sunlight nlohmann_json CLI11::CLI11 ${LUA_LIBRARIES} sodium miniz)

#
# Feeds --version/-v (main.cpp) - PROJECT_VERSION comes from this file's own
# project(scarab VERSION ...) above; SCARAB_SUNLIGHT_VERSION is the same
# variable the sunlight FetchContent_Declare's GIT_TAG already reads, so the
# printed sunlight version always matches what was actually fetched.
#
target_compile_definitions(scarab PRIVATE
  SCARAB_VERSION="${PROJECT_VERSION}"
  SCARAB_SUNLIGHT_VERSION="${SCARAB_SUNLIGHT_VERSION}"
  SCARAB_CONTENT_KEY="${SCARAB_CONTENT_KEY}")

#
# scarab_copy_binaries (below) flattens scarab + every one of its own runtime
# dependency dylibs/shared objects into one directory. CMake's own default
# rpath handling is correct for running straight out of the build tree (each
# binary's baked-in rpath points at its dependencies' real absolute build
# locations) but that default does NOT survive relocation - and since Scarab
# has no install() step (see the commented-out stub near the end of this
# file), the build output *is* what release.yml packages verbatim. Confirmed
# live (2026-08-29): a released macOS archive's scarab executable failed to
# even start once extracted anywhere else - `dyld: Library not loaded:
# @rpath/libsunlight.dylib`, `otool -l` showing its LC_RPATH entries were the
# CI runner's own absolute /Users/runner/work/... build paths. sunlight and
# tmx (both FetchContent'd, not something this file defines) turned out to
# have the identical problem for the same reason - their own build-time
# rpath entries point at THEIR OWN dependencies' build locations (sunlight ->
# raylib/tmx/libxml2; tmx -> libxml2). raylib/libxml2 needed no fix - neither
# has any rpath entry pointing at another FetchContent'd target's build dir.
#
# The fix: force every affected target's rpath to @loader_path (macOS) /
# $ORIGIN (Linux) - "look for my dependencies in my own directory" - which is
# exactly where scarab_copy_binaries puts them. BUILD_WITH_INSTALL_RPATH is
# what makes this apply directly to the BUILD output (there being no real
# install() step for it to apply to instead). Windows needs no equivalent -
# DLL search order defaults to "next to the .exe" with no baked-in-path
# mechanism to fix.
#
if(APPLE)
  set_target_properties(scarab sunlight tmx PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set_target_properties(scarab sunlight tmx PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN")
endif()

#
# On Unix, raylib/tmx/LibXml2 (unlike sunlight, which sets no VERSION/
# SOVERSION) each build as a *real* versioned shared object plus one or more
# symlinks - e.g. raylib: libraylib.5.5.0.dylib (real file) <-
# libraylib.550.dylib (SOVERSION symlink, matching the dylib's own embedded
# install name/ID - what every OTHER binary's @rpath reference actually
# looks for) <- libraylib.dylib (unversioned symlink). $<TARGET_FILE:X> only
# ever resolves to the real, fully-versioned file - copying just that one
# leaves the SOVERSION-symlink name missing from the flattened output
# directory, which the @loader_path rpath fix above then can't find (a real,
# previously-masked bug: the old broken absolute-path rpath happened to
# search the *original* build directory instead, where the symlinks still
# existed alongside it - see this file's own rpath comment above for the
# full story). $<TARGET_SONAME_FILE:X> is the generator expression CMake
# provides specifically for this - resolves to the symlink name the target's
# own SOVERSION/embedded ID actually uses, uniformly across ELF's SONAME and
# Mach-O's install name. Harmless no-op duplicate for sunlight (no
# versioning set, so its SONAME file and TARGET_FILE are identical).
#
# $<TARGET_SONAME_FILE:X> is a HARD CMake configure-time error on Windows
# ("TARGET_SONAME_FILE is not allowed for DLL target platforms") - confirmed
# live, broke Windows configure entirely the first time this was added.
# Windows DLLs have no SONAME/versioned-symlink concept at all, so these
# extra copies are Unix-only (if(NOT WIN32)), added as separate POST_BUILD
# commands on the already-defined target rather than folded into its main
# COMMAND list, matching this file's own existing pattern for the
# Windows-only Iconv/zlib DLL copies further below.
#
add_custom_target(scarab_copy_binaries
COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LibXml2> $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:raylib> $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:tmx> $<TARGET_FILE_DIR:scarab>
COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sunlight> $<TARGET_FILE_DIR:scarab>
COMMENT "Copying Scarab's own runtime dependency binaries" VERBATIM)

if(NOT WIN32)
  add_custom_command(TARGET scarab_copy_binaries POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_SONAME_FILE:LibXml2> $<TARGET_FILE_DIR:scarab>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_SONAME_FILE:raylib> $<TARGET_FILE_DIR:scarab>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_SONAME_FILE:tmx> $<TARGET_FILE_DIR:scarab>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_SONAME_FILE:sunlight> $<TARGET_FILE_DIR:scarab>
    COMMENT "Copying SOVERSION-symlink names (needed by @loader_path/$ORIGIN rpath resolution)" VERBATIM)
endif()

if(SCARAB_LIBSODIUM_SHARED)
  # Only needed when SCARAB_LIBSODIUM_SHARED actually built a real
  # libsodium.dylib/.so/.dll - the default static build has nothing
  # separate to copy at all, it's already inside scarab itself. sodium
  # sets no VERSION/SOVERSION target property (confirmed directly in
  # it's own CMakeLists.txt, same as sunlight - unlike raylib/tmx/
  # LibXml2), so unlike those three, no $<TARGET_SONAME_FILE:sodium>
  # companion copy is needed here at all.
  add_custom_command(TARGET scarab_copy_binaries POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sodium> $<TARGET_FILE_DIR:scarab>
    COMMENT "Copying libsodium's own shared library (SCARAB_LIBSODIUM_SHARED=ON)" VERBATIM)
endif()

# tools/pack.lua (the content-encryption packaging tool - see
# tools/README.md) copied next to the built executable,
# unconditionally, on every platform - this is what makes `scarab --pack
# <config.json>` (main.cpp) work from ANY directory, not just this repo's
# own root: main.cpp resolves the packer's own entry script as an
# APP_DIR-anchored absolute path (<APP_DIR>/tools/pack.lua), which only
# exists at runtime because of this copy. Whole tools/ directory copied
# (not just pack.lua) so pack-config.example.json/README.md ship
# alongside it too. Same $<TARGET_FILE_DIR:scarab> destination pattern as
# every other copy in this target, for the same single-config-generator
# reason documented above scarab_copy_binaries itself. This also means
# any downstream project consuming scarab via FetchContent gets its own
# copy of the packer for free, from its own build output directory -
# CMAKE_CURRENT_SOURCE_DIR here always refers to scarab's own source
# tree regardless of who FetchContent'd it.
add_custom_command(TARGET scarab_copy_binaries POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/tools" $<TARGET_FILE_DIR:scarab>/tools
  COMMENT "Copying tools/ (content-encryption packaging tool)" VERBATIM)

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

# Installation
# INSTALL(
#   TARGETS scarab sunlight
#   RUNTIME_DEPENDENCIES
#   RUNTIME DESTINATION "${CMAKE_INSTALL_PATH}/bin"
# )
