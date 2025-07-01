# OpenSubdiv Lite Regressions

## Overview

Regression tests for [osd_lite](https://github.com/NVIDIA-RTX/OSD-Lite)

## Requirements

* Windows or Linux
* CMake 3.22
* C++ 20

## Build

1. Git clone this source tree
   ```bash
   git clone https://github.com/NVIDIA-RTX/OSD-Lite-Tutorials
   ```

2. Ensure you have a recent C++ std20 compiler such as the latest Visual Studio 2022
   (run the Visual Studio updater application if unsure), Clang or GCC

3. Create a 'build' folder 
   (at the top of the repository, names such as 'build' or '_build' are recommended)

3. Use CMake to configure the build.
   * CMake GUI : make sure you set the build folder ! ("Where to build the binaries" line)

   * You can also use the command line or scripts:
     ```bash
     cmake_cmd="C:/Program\ Files/CMake/bin/cmake.exe"
     cmd="$cmake_cmd
          -G 'Visual Studio 17 2022 -A x64'
          .."
     echo $cmd
     eval $cmd
     ```

5. Build : either open the solution in MSVC or use the CMake command line builder
   ```bash
   cmake --build . --config Release
   ```
> [!WARNING]
> 32 bits builds are not supported

## Running Tests

From the build directory, use the `ctest` command. Ex:
```bash
ctest -C Release
```

Alternatively: use the `build_and_run.sh` bash script (edit as necessary). It should work
out-of-the-box for git-bash on windows, as well as linux bash (including WSL).
