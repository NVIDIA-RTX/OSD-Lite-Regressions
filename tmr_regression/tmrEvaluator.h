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

#include <opensubdiv/far/topologyRefiner.h>
#include <opensubdiv/tmr/surfaceTableFactory.h>

using namespace OpenSubdiv;

namespace tess {
    struct Patch;
};

struct Options;

template <typename REAL> class TmrEvaluator {

public:

    typedef std::vector<REAL> TessCoordVector;

    typedef Vec3<REAL> Vec3Real;
    typedef std::vector<Vec3Real> Vec3RealVector;

public:

    struct Descriptor {
        Options const& options;
        Far::TopologyRefiner const& baseMesh;
        Vec3RealVector const& basePos;
        Vec3RealVector const& baseUVs;
    };

    TmrEvaluator(Descriptor const& desc);

    ~TmrEvaluator();

public:

    void Evaluate(Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results);

private:

    void evaluateVertex(Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results);
    void evaluateFaceVarying(Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results);
    void evaluateLinearFaceVarying(Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results);

    Descriptor _descriptor;

    struct TopologyCache;

    std::unique_ptr<TopologyCache> _topologyCache;

    std::unique_ptr<Tmr::SurfaceTable> _vtxSurfaceTable;
    
    std::unique_ptr<Tmr::SurfaceTable> _fvarSurfaceTable;
    std::unique_ptr<Tmr::LinearSurfaceTable> _linearFVarSurfaceTable;

    Vec3RealVector _basePos;
    Vec3RealVector _baseUVs;

    Vec3RealVector _patchPoints;

    int  _regFaceSize = 0;
};
