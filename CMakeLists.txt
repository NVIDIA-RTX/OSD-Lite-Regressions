#
#   Copyright 2024 NVIDIA
#
#   Licensed under the Apache License, Version 2.0 (the "Apache License")
#   with the following modification; you may not use this file except in
#   compliance with the Apache License and the following modification to it:
#   Section 6. Trademarks. is deleted and replaced with:
#
#   6. Trademarks. This License does not grant permission to use the trade
#      names, trademarks, service marks, or product names of the Licensor
#      and its affiliates, except as required to comply with Section 4(c) of
#      the License and to reproduce the content of the NOTICE file.
#
#   You may obtain a copy of the Apache License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the Apache License with the above modification is
#   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#   KIND, either express or implied. See the Apache License for the specific
#   language governing permissions and limitations under the Apache License.
#

cmake_minimum_required(VERSION 3.22)

project(osd_lite_regression VERSION 1.0.0)

#
# Compiler
#

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_COMPILER_IS_CLANGCC 1)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(CMAKE_COMPILER_IS_GNUCC 1)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
    set(CMAKE_COMPILER_IS_ICC 1)
endif()

if (MSVC)

    add_definitions(/W3 /WX /DNOMINMAX /D_USE_MATH_DEFINES /D_CRT_SECURE_NO_WARNINGS)

else()

    if(CMAKE_COMPILER_IS_ICC)
        add_definitions(-w2 -wd1572 -wd1418 -wd981 -wd383 -wd193 -wd444)
    elseif(CMAKE_COMPILER_IS_CLANGCC OR CMAKE_COMPILER_IS_GNUCC)
        add_definitions(-Wall -Wextra)
    endif()

    add_definitions(-Werror)

endif()

#
# OSD_lite
#

option(OSD_LITE_LINK_DYNAMIC "Enable dynamic linking against OSD_lite " OFF)

set(osd_lite_lib "osd_lite_static")

if(OSD_LITE_LINK_DYNAMIC)
    set(osd_lite_lib "osd_lite_shared")
endif()

if (NOT OSD_lite_ROOT)

    if (NOT OSD_lite_git_tag)
        set(OSD_lite_git_tag "main" CACHE STRING "Git tag used to fetch OSD_lite")
    endif()

    include(FetchContent)
    FetchContent_Declare(OSD_lite
        GIT_REPOSITORY https://github.com/NVIDIA-RTX/OSD-Lite
        GIT_TAG "${OSD_lite_git_tag}"
    )
    FetchContent_MakeAvailable(OSD_lite)

else()

    find_package(OSD_lite REQUIRED)

    set(osd_lite_lib "osd::${osd_lite_lib}")

endif()

#
# Regressions
#

include(GNUInstallDirs)
include(CTest)

set(REGRESSION_FOLDER_NAME "Regression Tests")

add_subdirectory(common)
add_subdirectory(hbr)

add_subdirectory(hbr_regression)
add_subdirectory(far_regression)
add_subdirectory(far_perf)
add_subdirectory(bfr_evaluate)
add_subdirectory(tmr_regression)

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tmr_regression)
endif()
