#[=======================================================================[
SPDX-License-Identifier: GPL-2.0-only
SPDX-FileCopyrightText: © 2021 Avinal Kumar <avinal.xlvii@gmail.com>
#]=======================================================================]

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(nomos LANGUAGES C)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/SetDefaults.cmake)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/agent)

generate_version()

install(DIRECTORY ui
    DESTINATION ${FO_MODDIR}/${PROJECT_NAME}
    COMPONENT nomos
    FILES_MATCHING
    PATTERN *.php)

install(FILES nomos.conf "${CMAKE_CURRENT_BINARY_DIR}/VERSION"
    DESTINATION ${FO_MODDIR}/${PROJECT_NAME}
    COMPONENT nomos)

add_symlink()

if(TESTING)
    set(VERSIONFILE "VERSION")
    configure_file(${FO_CMAKEDIR}/TestInstall.make.in ${CMAKE_CURRENT_BINARY_DIR}/TestInstall.make
        NEWLINE_STYLE LF @ONLY)
    enable_testing()
    add_subdirectory(agent_tests)
endif(TESTING)
