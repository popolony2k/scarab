#
# CMake file for game development engine applicattion and libraries.
# Copyright (c) since 2021 by PopolonY2k
#

cmake_minimum_required(VERSION 3.14)

project(pog-games-distribution)

include(CMakeDependentOption)
include(GNUInstallDirs)


###################################################################
# Setup Game engine libraries and games projects
###################################################################

add_subdirectory( sunlight )
add_subdirectory( games/caravellius )
