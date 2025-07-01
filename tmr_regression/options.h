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

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <vector>

struct ShapeDesc;

struct Options {

    void initialize(int argc, char const** argv);

    enum PrintMask {
        kGeneralInfo = 0x1,
        kComparisonOptions = 0x2,
        kEvaluationOptions = 0x4,
        kOutputOptions = 0x8,
        kAll = 0xFF
    };

    void print(FILE* f, PrintMask mask = PrintMask::kAll) const;

    std::time_t timeStamp = std::time(nullptr);

    std::vector<ShapeDesc> shapes;

    enum class ShapeSet : uint8_t {
        kNone = 0,
        kCatmarkSet,
        kLoopSet,
        kAllSets
    } shapeSet = ShapeSet::kAllSets;

    enum class Scheme : uint8_t {
        kCatmark = 0,
        kLoop
    } scheme = Scheme::kCatmark;

    enum class VtxBoundary : uint8_t {
        kDefault = 0,
        kOverride_None,
        kOverride_EdgeOnly,
        KOverride_EdgeCorner,
    } vtxBoundary = VtxBoundary::kDefault;

    enum class FVarBoundary : uint8_t {
        kDefault = 0,
        kOverride_LinearNone,         // "edge only"
        kOverride_LinearCornersOnly,  // sharpen corners only
        kOverride_LinearCornersPlus1, // corners +1 mode
        kOverride_LinearCornersPlus2, // corners +2 mode
        kOverride_LinearBoundaries,   // "always sharp"
        kOverride_LinearAll,          // "bilinear"
    } fvarBoundary = FVarBoundary::kOverride_LinearAll;

    uint32_t fullBatchTesting : 1 = false;
    
    uint32_t ignoreKnownFailures : 1 = false;

    uint32_t doublePrecision : 1 = false;

    uint32_t evaluateD1 : 1 = true;
    uint32_t evaluateD2 : 1 = false;
    uint32_t evaluateUV : 1 = true;

    uint32_t ignoreVtx : 1 = false;

    uint32_t multi_threaded : 1 = true;
    uint32_t printProgress  : 1 = true;
    uint32_t printSummary   : 1 = true;
    
    uint32_t leftHanded : 1 = false;

    uint8_t isolationSharp = 6;
    uint8_t isolationSmooth = 2;

    uint32_t tessRate = 100;

    double tolerance = 0.00005f;
    double uvTolerance = 0.002f;

    std::filesystem::path statisticsFilePath;

    enum class MayaLog : uint8_t {
        kNever = 0,
        kFailure,
        kAlways,
    } mayaLog = MayaLog::kNever;

    std::filesystem::path mayaLogPath = [this]() {
        char buf[100];
        std::strftime(buf, sizeof(buf), "mayaLog_%m_%d_%Y-%H_%M_%S", std::localtime(&timeStamp));
        return std::filesystem::current_path() / buf; 
    }();
};

