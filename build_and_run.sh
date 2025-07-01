#!/usr/bin/bash

#
# automation script to build & run the regressions
#

# tested on:
#   - MinGW64 (Windows) : MSVC 2022
#   - WSL (Ubuntu) : gcc 11 / clang 19

project_dir="${PWD}"
build_dir="_build"

osd_lite_git_tag="main"
osd_lite_dir=""

cmake_cmd=`command -v "cmake"`
cpack_cmd=`command -v "cpack"`
ctest_cmd=`command -v "ctest"`

message () {
    local msg=$1
    local green_color="\e[92m"
    local end_color="\e[0m"
    echo -e "${green_color}== ${msg} ==${end_color}"
}

cmake_get_generator () {
    if [[  "$OSTYPE" == "win32" || "$OSTYPE" == "msys" ]]; then
        local cmake_generator="'Visual Studio 17 2022' -A x64"
    elif [[ "$OSTYPE" == "linux-gnu"* || "$OSTYPE" == "cygwin" || "$OSTYPE" == "freebsd"* ]]; then
        local cmake_generator="'Unix Makefiles'"
    fi
    echo "${cmake_generator}"
}

cmake_get_build_options () {
    if [[  "$OSTYPE" == "win32" || "$OSTYPE" == "msys" ]]; then
        local cmake_build_options="-- -v:minimal -m:16"
    elif [[ "$OSTYPE" == "linux-gnu"* || "$OSTYPE" == "cygwin" || "$OSTYPE" == "freebsd"* ]]; then
        local cmake_build_options="-- -j 16"
    fi
    echo "${cmake_build_options}"
}

cmake_configure () {

    local build_type=$1

    message "Configure"

    local cmake_generator="$(cmake_get_generator)"

    cmd="\"$cmake_cmd\" -G ${cmake_generator} "
    
    cmd+="-D \"CMAKE_INSTALL_PREFIX:string=../../${inst_dir}\" "

    if [[ ! -z "${osd_lite_git_tag}" ]]; then
        cmd+="-D \"OSD_lite_git_tag:string=${osd_lite_git_tag}\" "
    elif [[ ! -z "${osd_lite_root_dir}" ]]; then
        cmd+="-D \"OSD_lite_ROOT:string=${osd_lite_root_dir}\" "
    fi

    if [[ "$cmake_generator" == "'Unix Makefiles'" ]]; then
        cmd+="-D \"CMAKE_BUILD_TYPE:string=Release\" "
    fi

    cmd+=".."
    echo $cmd
    eval $cmd
}

cmake_build () {

    local build_type=$1

    message "Build (${build_type})"

    local cmake_build_options="$(cmake_get_build_options)"

    local cmd="\"$cmake_cmd\" --build . --config ${build_type} ${cmake_build_options}"
    echo $cmd
    eval $cmd
}

cmake_test () {

    local build_type=$1

    message "Test"
    
    local cmd="\"$ctest_cmd\" -C ${build_type}"
    echo $cmd
    eval $cmd
}

main () {

    if [ ! -d "${project_dir}/${build_dir}" ]; then
        mkdir "${project_dir}/${build_dir}"
    fi

    build_type="Release"

    cd "${project_dir}/${build_dir}"

    cmake_configure "${build_type}"

    cmake_build "${build_type}"

    cmake_test "${build_type}"

    cd "${project_dir}"
}

main "$@"

