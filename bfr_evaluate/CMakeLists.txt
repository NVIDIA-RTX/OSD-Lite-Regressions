#
#   Copyright 2021 Pixar
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

set(src_files
    main.cpp
    bfrSurfaceEvaluator.cpp
    bfrSurfaceEvaluator.h
    farPatchEvaluator.cpp
    farPatchEvaluator.h
    init_shapes_all.h    
    init_shapes.h
    types.h
)

add_executable(bfr_evaluate ${src_files})
target_link_libraries(bfr_evaluate common_lib)
set_target_properties(bfr_evaluate PROPERTIES FOLDER ${REGRESSION_FOLDER_NAME})

if (MSVC AND ${OSD_LITE_LINK_DYNAMIC})
    add_custom_command(TARGET bfr_evaluate POST_BUILD COMMAND 
        ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:bfr_evaluate> $<TARGET_FILE_DIR:bfr_evaluate> COMMAND_EXPAND_LISTS)
endif()

install(TARGETS bfr_evaluate DESTINATION "${CMAKE_BINDIR_BASE}")

add_test(NAME bfr_evaluate_pos COMMAND "$<TARGET_FILE:bfr_evaluate>" -all -silent -l 3 -pass 2 -d1)
add_test(NAME bfr_evaluate_uv1 COMMAND "$<TARGET_FILE:bfr_evaluate>" -all -silent -l 3 -pass 0 -skippos -uv -uvint 1)
add_test(NAME bfr_evaluate_uv5 COMMAND "$<TARGET_FILE:bfr_evaluate>" -all -silent -l 3 -pass 0 -skippos -uv -uvint 5)
