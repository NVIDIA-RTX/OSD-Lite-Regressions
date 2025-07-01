#
#   Copyright 2015 Pixar
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
    far_perf.cpp
)

add_executable(far_perf ${src_files})
target_link_libraries(far_perf common_lib)
set_target_properties(far_perf PROPERTIES FOLDER ${REGRESSION_FOLDER_NAME})

if (MSVC AND ${OSD_LITE_LINK_DYNAMIC})
    add_custom_command(TARGET far_perf POST_BUILD COMMAND 
        ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:far_perf> $<TARGET_FILE_DIR:far_perf> COMMAND_EXPAND_LISTS)
endif()

install(TARGETS far_perf DESTINATION "${CMAKE_INSTALL_BINDIR}")

add_test(NAME far_perf COMMAND "$<TARGET_FILE:far_perf>")
