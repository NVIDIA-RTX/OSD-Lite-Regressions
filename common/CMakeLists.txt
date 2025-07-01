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

set(common_source_files
    far_utils.cpp
    shape_utils.cpp
    tess_spaced.cpp
    tess_uniform.cpp
)

set(common_header_files
    box.h
    far_utils.h
    mesh_loader.h
    objWriter.h
    shape_utils.h
    stopwatch.h
    tess.h
)

add_library(common_lib STATIC ${common_source_files} ${common_header_files})

target_link_libraries(common_lib PUBLIC ${osd_lite_lib})
target_include_directories(common_lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_target_properties(common_lib PROPERTIES FOLDER "${REGRESSION_FOLDER_NAME}/Common")
