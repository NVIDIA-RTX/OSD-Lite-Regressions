#
#   Copyright 2016 Nvidia
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

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANGCC)
    add_definitions(-Wno-unused-function -Wno-missing-field-initializers -Wno-missing-designated-field-initializers)
endif()

set(src_files 
    regression.cpp
    regressionTask.cpp
    regressionTask.h
    init_shapes.cpp
    init_shapes.h
    mayaLogger.h
    mayaLogger.cpp
    options.cpp
    options.h
    farEvaluator.cpp
    farEvaluator.h
    tmrEvaluator.cpp
    tmrEvaluator.h
    types.h)

add_executable(tmr_regression ${src_files})
target_link_libraries(tmr_regression common_lib)
set_target_properties(tmr_regression PROPERTIES FOLDER ${REGRESSION_FOLDER_NAME})

if (CMAKE_COMPILER_IS_CLANGCC OR CMAKE_COMPILER_IS_GNUCC)
    # gcc / clang rely on TBB for std::for_each other implementations of <execution>
    find_package(TBB REQUIRED)
    target_link_libraries(tmr_regression TBB::tbb)
endif()

if (MSVC AND ${OSD_LITE_LINK_DYNAMIC})
    add_custom_command(TARGET tmr_regression POST_BUILD COMMAND 
        ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:tmr_regression> $<TARGET_FILE_DIR:tmr_regression> COMMAND_EXPAND_LISTS)
endif()

install(TARGETS tmr_regression DESTINATION "${CMAKE_INSTALL_BINDIR}")

# note : -knownFailures allows CTest to mark the regression as a success, even though some tests 
#        are flagged in a list as known failures. These tests still record errors in the log file
#        (see regresion.cpp)
add_test(NAME tmr_regression COMMAND "$<TARGET_FILE:tmr_regression>" -full -knownFailures -noprog -nosum)
