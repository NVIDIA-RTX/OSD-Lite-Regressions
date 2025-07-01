#
#   Copyright 2013 Pixar
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
    # hbr does some in-place construction and pointer hackery
    add_definitions(-Wno-switch -Wno-invalid-offsetof -Wno-strict-aliasing)
endif()

#
# baseline generation tool
#

add_executable(hbr_baseline baseline.cpp)
target_link_libraries(hbr_baseline common_lib)
set_target_properties(hbr_baseline PROPERTIES FOLDER ${REGRESSION_FOLDER_NAME})

add_executable(hbr_regression main.cpp)
target_link_libraries(hbr_regression common_lib hbr_lib)
target_compile_definitions(hbr_regression PRIVATE HBR_BASELINE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/baseline/")
set_target_properties(hbr_regression PROPERTIES FOLDER ${REGRESSION_FOLDER_NAME})
add_dependencies(hbr_regression hbr_baseline)

install(TARGETS hbr_baseline DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(TARGETS hbr_regression DESTINATION "${CMAKE_INSTALL_BINDIR}")

add_test(NAME hbr_regression COMMAND "$<TARGET_FILE:hbr_regression>")
