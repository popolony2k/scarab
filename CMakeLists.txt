#
# CMake file for building sunlight game engine library and related games.
# Copyright (c) since 2021 by PopolonY2k
#

cmake_minimum_required(VERSION 3.24)

project(pog-games-distribution)

include(CMakeDependentOption)
include(GNUInstallDirs)


###################################################################
# Setup Game engine libraries and games projects
###################################################################

add_subdirectory( games/caravellius )
