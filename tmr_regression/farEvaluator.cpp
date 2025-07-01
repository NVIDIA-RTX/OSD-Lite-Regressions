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

#include "./farEvaluator.h"

#include "./options.h"

#include <common/tess.h>

#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/far/primvarRefiner.h>


template <typename REAL> FarEvaluator<REAL>::FarEvaluator(Descriptor const& desc) : _descriptor(desc) {

    Options const& options = desc.options;

    Far::PatchTableFactory::Options::EndCapType endCapType = Far::PatchTableFactory::Options::ENDCAP_GREGORY_BASIS;
    int primaryLevel = options.isolationSharp;
    int secondaryLevel = options.isolationSmooth;
    bool useInfSharpPatch = true;
    bool hasUVs = !desc.baseUVs.empty();
    int  fvarChannel = 0;

    Far::TopologyRefiner const& refiner = desc.baseMesh;

    _regFaceSize = Sdc::SchemeTypeTraits::GetRegularFaceSize(refiner.GetSchemeType());

    Far::PatchTableFactory::Options patchOptions(primaryLevel);
    patchOptions.SetPatchPrecision<REAL>();
    patchOptions.SetFVarPatchPrecision<REAL>();
    patchOptions.useInfSharpPatch = useInfSharpPatch;
    patchOptions.generateLegacySharpCornerPatches = false;
    patchOptions.shareEndCapPatchPoints = false;
    patchOptions.endCapType = endCapType;
    patchOptions.generateFVarTables = hasUVs;
    patchOptions.numFVarChannels = hasUVs ? 1 : 0;
    patchOptions.fvarChannelIndices = &fvarChannel;
    patchOptions.generateFVarLegacyLinearPatches = false;
    patchOptions.generateVaryingTables = false;

    Far::TopologyRefiner::AdaptiveOptions refineOptions =
        patchOptions.GetRefineAdaptiveOptions();
    refineOptions.SetIsolationLevel(primaryLevel);
    refineOptions.SetSecondaryLevel(secondaryLevel);
    refineOptions.useInfSharpPatch = useInfSharpPatch;

    std::unique_ptr<Far::TopologyRefiner> patchRefiner(
        Far::TopologyRefinerFactory<Far::TopologyDescriptor>::Create(refiner));

    patchRefiner->RefineAdaptive(refineOptions);

    _patchTable.reset(Far::PatchTableFactory::Create(*patchRefiner, patchOptions));

    _patchMap = std::make_unique<Far::PatchMap>(*_patchTable);

    Far::TopologyLevel const& baseLevel = refiner.GetLevel(0);

    int numBasePoints = baseLevel.GetNumVertices();
    int numRefinedPoints = patchRefiner->GetNumVerticesTotal() - numBasePoints;
    int numLocalPoints = _patchTable->GetNumLocalPoints();

    _patchPos.resize(numBasePoints + numRefinedPoints + numLocalPoints);

    std::memcpy(_patchPos.data(), desc.basePos.data(), numBasePoints * sizeof(Vec3Real));

    int numBaseUVs = 0;
    int numRefinedUVs = 0;
    int numLocalUVs = 0;

    if (hasUVs) {
        numBaseUVs = baseLevel.GetNumFVarValues();
        numRefinedUVs = patchRefiner->GetNumFVarValuesTotal() - numBaseUVs;
        numLocalUVs = _patchTable->GetNumLocalPointsFaceVarying();

        _patchUVs.resize(numBaseUVs + numRefinedUVs + numLocalUVs);

        std::memcpy(_patchUVs.data(), desc.baseUVs.data(), numBaseUVs * sizeof(Vec3Real));
    }

    if (numRefinedPoints) {

        Far::PrimvarRefinerReal<REAL> primvarRefiner(*patchRefiner);

        Vec3Real const* srcP = &_patchPos[0];
        Vec3Real* dstP = &_patchPos[numBasePoints];

        Vec3Real const* srcUV = hasUVs ? &_patchUVs[0] : 0;
        Vec3Real* dstUV = hasUVs ? &_patchUVs[numBaseUVs] : 0;

        for (int level = 1; level < patchRefiner->GetNumLevels(); ++level) {
            primvarRefiner.Interpolate(level, srcP, dstP);
            srcP = dstP;
            dstP += patchRefiner->GetLevel(level).GetNumVertices();

            if (hasUVs) {
                primvarRefiner.InterpolateFaceVarying(level, srcUV, dstUV);
                srcUV = dstUV;
                dstUV += patchRefiner->GetLevel(level).GetNumFVarValues();
            }
        }
    }
    if (numLocalPoints) {
        _patchTable->GetLocalPointStencilTable<REAL>()->UpdateValues(
            &_patchPos[0], &_patchPos[numBasePoints + numRefinedPoints]);
    }
    if (hasUVs && numLocalUVs) {
        _patchTable->GetLocalPointFaceVaryingStencilTable<REAL>()->UpdateValues(
            &_patchUVs[0], &_patchUVs[numBaseUVs + numRefinedUVs]);
    }
}

template <typename REAL> bool FarEvaluator<REAL>::FaceHasLimit(Far::Index baseFace) const {
    return !_descriptor.baseMesh.GetLevel(0).IsFaceHole(baseFace);
}

template <typename REAL> void FarEvaluator<REAL>::Evaluate(
    Far::Index surfIndex, tess::Patch const& tessCoords, EvalResults<REAL>& results) const {

    auto level = _descriptor.baseMesh.GetLevel(0);

    int numCoords = (int)tessCoords.numVertices();
    results.Resize(numCoords);

    for (int i = 0; i < numCoords; ++i) {

        float s = tessCoords.u[i];
        float t = tessCoords.v[i];

        Far::PatchTable::PatchHandle const* patchHandle = _patchMap->FindPatch(surfIndex, s, t);
        assert(patchHandle);

        if (results.evalP) {

            REAL wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], wDvv[20];

            if (!results.eval1stDeriv)
                _patchTable->EvaluateBasis(*patchHandle, s, t, wP);
            else if (!results.eval2ndDeriv)
                _patchTable->EvaluateBasis(*patchHandle, s, t, wP, wDu, wDv);
            else
                _patchTable->EvaluateBasis(*patchHandle, s, t, wP, wDu, wDv, wDuu, wDuv, wDvv);       

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

            Far::ConstIndexArray cvIndices = _patchTable->GetPatchVertices(*patchHandle);

            for (int cv = 0; cv < cvIndices.size(); ++cv) {
                P->AddWithWeight(_patchPos[cvIndices[cv]], wP[cv]);
                if (results.eval1stDeriv) {
                    Du->AddWithWeight(_patchPos[cvIndices[cv]], wDu[cv]);
                    Dv->AddWithWeight(_patchPos[cvIndices[cv]], wDv[cv]);
                    if (results.eval2ndDeriv) {
                        Duu->AddWithWeight(_patchPos[cvIndices[cv]], wDuu[cv]);
                        Duv->AddWithWeight(_patchPos[cvIndices[cv]], wDuv[cv]);
                        Dvv->AddWithWeight(_patchPos[cvIndices[cv]], wDvv[cv]);
                    }
                }
            }
        }

        if (results.evalUV) {

            REAL wUV[20];
        
            _patchTable->EvaluateBasisFaceVarying(*patchHandle, s, t, wUV);

            Vec3Real& UV = results.uv[i];

            UV.Clear();

            Far::ConstIndexArray cvIndices = _patchTable->GetPatchFVarValues(*patchHandle);

            for (int cv = 0; cv < cvIndices.size(); ++cv)
                UV.AddWithWeight(_patchUVs[cvIndices[cv]], wUV[cv]);
        }
    }
}

template class FarEvaluator<float>;
template class FarEvaluator<double>;
