//
//   Copyright 2016 Nvidia
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#pragma once

#include "./types.h"
#include "./init_shapes.h"

#include <common/stopwatch.h>

#include <cstdio>
#include <memory>

struct Options;

// thread-safe regression task
class RegressionTask {

public:

    bool execute();

    void printPass(FILE* f = stdout) const;
    void printTimes(FILE* f = stdout) const;
    void printMeshDelta(FILE* f = stdout) const;

    // task

    ShapeDesc const* shapeDesc = nullptr;

    Options const* options = nullptr;

    // results

    MeshDelta<float> meshDelta;

    Stopwatch farBuildTime;
    Stopwatch farEvalTime;
    Stopwatch tmrBuildTime;
    Stopwatch tmrEvalTime;
    Stopwatch execTime;

    bool isKnownFailure = false;

    struct Mesh;

    static void populateTessCache(uint8_t tessRate);

private:

    bool execute(bool logMaya);

    std::unique_ptr<Mesh> createMesh(ShapeDesc const& shapeDesc) const;

};
