#[=======================================================================[
SPDX-License-Identifier: GPL-2.0-only
SPDX-FileCopyrightText: © 2021 Avinal Kumar <avinal.xlvii@gmail.com>
#]=======================================================================]

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(buckets LANGUAGES C)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/SetDefaults.cmake)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/agent)
generate_version()
install(DIRECTORY ui
    DESTINATION ${FO_MODDIR}/${PROJECT_NAME}
    COMPONENT buckets
    FILES_MATCHING 
    PATTERN *.php)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VERSION" buckets.conf 
    DESTINATION ${FO_MODDIR}/${PROJECT_NAME}
    COMPONENT buckets)

add_symlink()
