#
# CMake file for game development engine applicattion and libraries.
# Copyright (c) since 2021 by PopolonY2k
#

cmake_minimum_required(VERSION 2.8.11)

project(pog-games-distribution)

include(CMakeDependentOption)
include(GNUInstallDirs)

add_subdirectory( sunlight )
add_subdirectory( games/hypex )
