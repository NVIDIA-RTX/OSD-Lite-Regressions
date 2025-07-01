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

#include "./tmrEvaluator.h"
#include "./options.h"

#include <opensubdiv/far/patchBasis.h>
#include <common/tess.h>

#include <cassert>
#include <map>

template <typename REAL> inline void quadDomainRotate(int rot, Vec3<REAL>& du, Vec3<REAL>& dv) {
    switch (rot) {
        case 0: break;
        case 1: { Vec3<REAL> a = dv; dv = du; du = -a; } break;
        case 2: { du = -du; dv = -dv; } break;
        case 3: { Vec3<REAL> a = dv; dv = -du; du = a; } break;
        default: assert(0);
    }
}

template <typename REAL> inline void triangleDomainRotate(int rot, Vec3<REAL>& du, Vec3<REAL>& dv) {
    switch (rot) {
        case 0: break;
        case 1: { Vec3<REAL> a = du; du = -dv; dv = a + du; } break;
        case 2: { Vec3<REAL> a = dv; dv = -du; du = a + dv; } break;
        default: assert(0);
    }
}

template <typename REAL> void applyDomainRotation(
    tess::DomainMode domain, int rot, Vec3<REAL>& du, Vec3<REAL>& dv) {

    using enum tess::DomainMode;
    switch (domain) {
        case QUAD: quadDomainRotate<REAL>(rot, du, dv); break;
        case TRIANGLE: triangleDomainRotate<REAL>(rot, du, dv); break;
        default: assert(0);
    }
}

template <typename REAL> inline void quadDomainRotate(int rot, Vec3<REAL>& duu, Vec3<REAL>& duv, Vec3<REAL>& dvv) {
    switch (rot) {
        case 0: break;
        case 1: { std::swap(duu, dvv); duv = -duv; } break;
        case 2: break;
        case 3: { std::swap(duu, dvv); duv = -duv; } break;
        default: assert(0);
    }
}

template <typename REAL> void applyDomainRotation(
    tess::DomainMode domain, int rot, Vec3<REAL>& duu, Vec3<REAL>& duv, Vec3<REAL>& dvv) {

    using enum tess::DomainMode;
    switch (domain) {
        case QUAD: quadDomainRotate<REAL>(rot, duu, duv, dvv); break;
        case TRIANGLE: break; // no rotations necessary apparently ???
        default: assert(0);
    }
}

template<typename REAL> struct TmrEvaluator<REAL>::TopologyCache {

    union Key {
        Tmr::TopologyMap::Traits traits;
        uint8_t value;
    };

    Tmr::TopologyMap& get(Tmr::TopologyMap::Traits traits) {
        Key key = { .traits = traits };
        if (auto it = topoMaps.find(key.value); it != topoMaps.end())
            return *it->second;
        auto [it, done] = topoMaps.emplace(key.value,
            std::make_unique<Tmr::TopologyMap>(key.traits));
        return *it->second;
    }

    void clear() {
        topoMaps.clear();
    }

    int getNumPatchPointsMax() const {
        int npoints = 0;
        for (auto const& tmap : topoMaps)
            npoints = std::max(npoints, tmap.second->GetNumPatchPointsMax());
        return npoints;
    }

    std::map<uint8_t, std::unique_ptr<Tmr::TopologyMap>> topoMaps;
};

template<typename REAL> TmrEvaluator<REAL>::TmrEvaluator(Descriptor const& desc) : _descriptor(desc) {

    Options const& options = desc.options;

    Tmr::EndCapType endCapType = Tmr::EndCapType::ENDCAP_GREGORY_BASIS;
    int primaryLevel = options.isolationSharp;
    int secondaryLevel = options.isolationSmooth;
    bool useInfSharpPatch = true;
    bool hasUVs = options.evaluateUV && !desc.baseUVs.empty();
    int  fvarChannel = 0;

    Far::TopologyRefiner const& refiner = desc.baseMesh;

    Sdc::SchemeType schemeType = refiner.GetSchemeType();
    Sdc::Options schemeOptions = refiner.GetSchemeOptions();

    _regFaceSize = Sdc::SchemeTypeTraits::GetRegularFaceSize(schemeType);

    _topologyCache = std::make_unique<TopologyCache>();

    Tmr::SurfaceTableFactory::Options surfOptions;
    surfOptions.planBuilderOptions.endCapType = endCapType;
    surfOptions.planBuilderOptions.isolationLevel = primaryLevel;
    surfOptions.planBuilderOptions.isolationLevelSecondary = secondaryLevel;
    surfOptions.planBuilderOptions.useSingleCreasePatch = false;
    surfOptions.planBuilderOptions.useInfSharpPatch = useInfSharpPatch;
    surfOptions.planBuilderOptions.useTerminalNode = true;
    surfOptions.planBuilderOptions.useDynamicIsolation = false;
    surfOptions.planBuilderOptions.orderStencilMatrixByLevel = false;
    surfOptions.planBuilderOptions.generateLegacySharpCornerPatches = false;

    Tmr::SurfaceTableFactory tableFactory;

    // vertex
    {
        Tmr::TopologyMap::Traits traits;
        traits.SetCompatible(schemeType, schemeOptions, endCapType);

        Tmr::TopologyMap& topologyMap = _topologyCache->get(traits);

        _vtxSurfaceTable = tableFactory.Create(refiner, topologyMap, surfOptions);
    }

    // face-varying
    if (hasUVs) {

        using enum Sdc::Options::FVarLinearInterpolation;  

        if (schemeOptions.GetFVarLinearInterpolation() == FVAR_LINEAR_ALL) {

            Tmr::LinearSurfaceTableFactory tableFactory;
            _linearFVarSurfaceTable = tableFactory.Create(refiner, fvarChannel);

        } else {

            Tmr::TopologyMap::Traits traits;
            traits.SetCompatible(schemeType, refiner.GetSchemeOptions(), endCapType, true);

            Tmr::TopologyMap& topologyMap = _topologyCache->get(traits);

            surfOptions.fvarChannel = fvarChannel;
            
            // note: parametric rotations should not be compensated here, or the limit
            // samples ordering will not match that of the Far evaluator samples
            surfOptions.depTable = nullptr; // _vtxSurfaceTable.get();

            _fvarSurfaceTable = tableFactory.Create(refiner, topologyMap, surfOptions);
        }
    }

    _patchPoints.resize(_topologyCache->getNumPatchPointsMax());

    _basePos = desc.basePos;
    _baseUVs = desc.baseUVs;
}

template<typename REAL> TmrEvaluator<REAL>::~TmrEvaluator() { }

template<typename REAL> void TmrEvaluator<REAL>::evaluateVertex(
    Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results) {

    assert(_regFaceSize == 4 || _regFaceSize == 3);
    tess::DomainMode domain = _regFaceSize == 4 ? tess::DomainMode::QUAD : tess::DomainMode::TRIANGLE;

    Tmr::TopologyMap const& topologyMap = _vtxSurfaceTable->topologyMap;

    Tmr::SurfaceDescriptor desc = _vtxSurfaceTable->GetDescriptor(surfIndex);
    assert(desc.HasLimit());

    int rot = desc.GetParametricRotation();

    Tmr::SubdivisionPlan const& plan = *topologyMap.GetSubdivisionPlan(desc.GetSubdivisionPlanIndex());

    //bool regular = plan.IsRegularFace();

    int numControlPoints = plan.GetNumControlPoints();

    Tmr::ConstIndexArray controlPoints = {
        _vtxSurfaceTable->GetControlPointIndices(surfIndex), numControlPoints };

    // seed the patchPoints with the 1-ring control point positions
    for (int i = 0; i < numControlPoints; ++i)
        _patchPoints[i] = _basePos[controlPoints[i]];

    plan.EvaluatePatchPoints<Vec3Real, Vec3Real>(
        _basePos.data(), controlPoints, _patchPoints.data() + numControlPoints);

    int numCoords = (int)tessCoords.numVertices();

    for (int i = 0; i < numCoords; ++i) {

        float s = tessCoords.u[i];
        float t = tessCoords.v[i];

        auto [u, v] = tess::rotateDomainInv(domain, rot, s, t);

        REAL wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], wDvv[20];

        unsigned char quadrant = 0;
        Tmr::SubdivisionPlan::Node node;
        
        if (!results.eval1stDeriv)
            node = plan.EvaluateBasis(u, v, wP, nullptr, nullptr, nullptr, nullptr, nullptr, &quadrant);
        else if (!results.eval2ndDeriv)
            node = plan.EvaluateBasis(u, v, wP, wDu, wDv, nullptr, nullptr, nullptr, &quadrant);
        else
            node = plan.EvaluateBasis(u, v, wP, wDu, wDv, wDuu, wDuv, wDvv, &quadrant);

        Vec3Real* P = &results.p[i];
        Vec3Real* Du = results.eval1stDeriv ? &results.du[i] : nullptr;
        Vec3Real* Dv = results.eval1stDeriv ? &results.dv[i] : nullptr;
        Vec3Real* Duu = results.eval2ndDeriv ? &results.duu[i] : nullptr;
        Vec3Real* Duv = results.eval2ndDeriv ? &results.duv[i] : nullptr;
        Vec3Real* Dvv = results.eval2ndDeriv ? &results.dvv[i] : nullptr;

        P->Clear();
        if (results.eval1stDeriv) {
            Du->Clear();
            Dv->Clear();
            if (results.eval2ndDeriv) {
                Duu->Clear();
                Duv->Clear();
                Dvv->Clear();
            }
        }

        int patchSize = node.GetPatchSize(quadrant);

        for (int j = 0; j < patchSize; ++j) {
            
            Tmr::Index patchPointIndex = node.GetPatchPoint(j, quadrant);

            P->AddWithWeight(_patchPoints[patchPointIndex], wP[j]);
            if (results.eval1stDeriv) {
                Du->AddWithWeight(_patchPoints[patchPointIndex], wDu[j]);
                Dv->AddWithWeight(_patchPoints[patchPointIndex], wDv[j]);
                if (results.eval2ndDeriv) {
                    Duu->AddWithWeight(_patchPoints[patchPointIndex], wDuu[j]);
                    Duv->AddWithWeight(_patchPoints[patchPointIndex], wDuv[j]);
                    Dvv->AddWithWeight(_patchPoints[patchPointIndex], wDvv[j]);
                }
            }
        }

        if (results.eval1stDeriv) {
            applyDomainRotation(domain, desc.GetParametricRotation(), *Du, *Dv);
        }
        if (results.eval2ndDeriv) {
            applyDomainRotation(domain, desc.GetParametricRotation(), *Duu, *Duv, *Dvv);
        }
    }
}


template<typename REAL> void TmrEvaluator<REAL>::evaluateLinearFaceVarying(
    Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results) {

    Tmr::LinearSurfaceTable const& surfaceTable = *_linearFVarSurfaceTable;

    Tmr::LinearSurfaceDescriptor desc = surfaceTable.GetDescriptor(surfIndex);
    assert(desc.HasLimit());

    Tmr::LocalIndex subface = desc.GetQuadSubfaceIndex();
    
    int numControlPoints = desc.GetFaceSize();

    Tmr::ConstIndexArray controlPoints = {
        surfaceTable.GetControlPointIndices(surfIndex), numControlPoints };

    for (int i = 0; i < numControlPoints; ++i)
        _patchPoints[i] = _baseUVs[controlPoints[i]];

    surfaceTable.EvaluatePatchPoints<Vec3Real, Vec3Real>(
        surfIndex, _baseUVs.data(), _patchPoints.data() + numControlPoints);

    int numCoords = (int)tessCoords.numVertices();

    for (int i = 0; i < numCoords; ++i) {

        float s = tessCoords.u[i];
        float t = tessCoords.v[i];

        REAL wUV[4];

        Vec3Real& UV = results.uv[i];

        UV.Clear();

        int patchSize = _regFaceSize;
        switch (patchSize) {
            case 3 : Far::internal::EvalBasisLinearTri<REAL>(s, t, wUV); break;
            case 4 : Far::internal::EvalBasisLinear<REAL>(s, t, wUV); break;
            default: assert(0);
        }

        for (int j = 0; j < _regFaceSize; ++j) {
            
            Tmr::Index pindex = desc.GetPatchPoint(j, numControlPoints, subface);

            UV.AddWithWeight(_patchPoints[pindex], wUV[j]);
        }
    }
}

template<typename REAL> void TmrEvaluator<REAL>::evaluateFaceVarying(
    Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results) {

    assert(_regFaceSize == 4 || _regFaceSize == 3);
    tess::DomainMode domain = _regFaceSize == 4 ? tess::DomainMode::QUAD : tess::DomainMode::TRIANGLE;

    Tmr::SurfaceTable const& surfaceTable = *_fvarSurfaceTable;

    Tmr::TopologyMap const& topologyMap = surfaceTable.topologyMap;

    Tmr::SurfaceDescriptor desc = surfaceTable.GetDescriptor(surfIndex);
    assert(desc.HasLimit());

    int rot = desc.GetParametricRotation();

    Tmr::SubdivisionPlan const& plan = *topologyMap.GetSubdivisionPlan(desc.GetSubdivisionPlanIndex());

    //bool regular = plan.IsRegularFace();

    int numControlPoints = plan.GetNumControlPoints();

    Tmr::ConstIndexArray controlPoints = {
        surfaceTable.GetControlPointIndices(surfIndex), numControlPoints };

    // seed the patchPoints with the 1-ring control point positions
    for (int i = 0; i < numControlPoints; ++i)
        _patchPoints[i] = _baseUVs[controlPoints[i]];

    plan.EvaluatePatchPoints<Vec3Real, Vec3Real>(
        _baseUVs.data(), controlPoints, _patchPoints.data() + numControlPoints);

    int numCoords = (int)tessCoords.numVertices();
    results.Resize(numCoords);

    for (int i = 0; i < numCoords; ++i) {

        float s = tessCoords.u[i];
        float t = tessCoords.v[i];

        auto [u, v] = tess::rotateDomainInv(domain, rot, s, t);

        REAL wUV[20];

        unsigned char quadrant = 0;
        Tmr::SubdivisionPlan::Node node;

        node = plan.EvaluateBasis(u, v, wUV, nullptr, nullptr, nullptr, nullptr, nullptr, &quadrant);

        Vec3Real& UV = results.uv[i];

        UV.Clear();

        int patchSize = node.GetPatchSize(quadrant);

        for (int j = 0; j < patchSize; ++j) {
            Tmr::Index pindex = node.GetPatchPoint(j, quadrant);
            UV.AddWithWeight(_patchPoints[pindex], wUV[j]);
        }
    }
}

template<typename REAL> void TmrEvaluator<REAL>::Evaluate(
    Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results) {

    results.Resize((int)tessCoords.numVertices());

    if (results.evalP)
        evaluateVertex(surfIndex, tessCoords, results);

    if (results.evalUV) {
        if (_fvarSurfaceTable)
            evaluateFaceVarying(surfIndex, tessCoords, results);
        else if (_linearFVarSurfaceTable)
            evaluateLinearFaceVarying(surfIndex, tessCoords, results);
    }

}

template class TmrEvaluator<float>;
template class TmrEvaluator<double>;