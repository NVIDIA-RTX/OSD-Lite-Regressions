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

#include <cstdio>
#include <filesystem>
#include <string>

class MayaLogger {

public:

    ~MayaLogger();

    void Initialize(std::filesystem::path const &filepath);

    template <typename REAL> void LogFace(int surfIndex, FaceDeltaVectors<REAL> const& deltaVecs);

    // log failed samples only or full Beta output
    static constexpr bool const logFullBeta = false;

private:

    template <typename REAL> void logPoints(char const* faceName,
        char const* nodePath, VectorDelta<REAL> const& pDelta);

    template <typename REAL> void logVecs(char const* faceName,
        char const* nodePath, VectorDelta<REAL> const& pDelta, VectorDelta<REAL> const& d1Delta);

    std::filesystem::path _filepath;
    FILE* _handle = nullptr;

    std::string _mayaDeltaPPath;
    
    std::string _mayaDeltaDuPath;
    std::string _mayaDeltaDvPath;
    
    std::string _mayaDeltaDuuPath;
    std::string _mayaDeltaDuvPath;
    std::string _mayaDeltaDvvPath;

    std::string _mayaDeltaUVPath;
};