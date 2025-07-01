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

#include "./regressionTask.h"
#include "./init_shapes.h"
#include "./mayaLogger.h"
#include "./farEvaluator.h"
#include "./tmrEvaluator.h"
#include "./options.h"

#include <opensubdiv/far/topologyRefinerFactory.h>

#include <common/tess.h>
#include <common/box.h>

using namespace OpenSubdiv;

struct TessCache {

    // uniform tessellation only, so we only need 2 tess patterns:
    // full res & half-res for irregular faces (T-junctions)

    tess::Patch patchQuad;
    tess::Patch patchQuadHalf;

    tess::Patch patchTri;
    tess::Patch patchTriHalf;

    std::pair<tess::Patch const&, tess::Patch const&> patches(tess::DomainMode domain) const {
        return domain == tess::DomainMode::TRIANGLE ?
            std::pair<tess::Patch const&, tess::Patch const&>{ patchTri, patchTriHalf }
        : std::pair<tess::Patch const&, tess::Patch const&>{ patchQuad, patchQuadHalf };
    }

    void populate(uint8_t level) {
        assert((level & 0x1) == 1);
        using enum tess::DomainMode;

        tess::uniform::tessellate(QUAD, level, patchQuad);
        tess::uniform::tessellate(QUAD, level / 2 + 1, patchQuadHalf);

        tess::uniform::tessellate(TRIANGLE, level, patchTri);
        tess::uniform::tessellate(TRIANGLE, level / 2 + 1, patchTriHalf);
    }
} _tessCache;

template <typename REAL, int n> inline REAL getRelativeTolerance(box<REAL, n> const& box, REAL scale) {
    
    static_assert(n > 1);

    auto diagonal = box.diagonal();
    
    REAL max = REAL(std::fabs(diagonal[0]));
    for (size_t i = 1; i < n; ++i)
        max = std::max(max, std::fabs(diagonal[i]));

    // if for some reason the AABB is smaller than the absolute scale, use
    // absolute scale
    return std::max(scale, max * scale);
}

static int countSurfaces(Far::TopologyRefiner const& refiner) {

    Vtr::internal::Level const& level = refiner.getLevel(0);

    int regFaceSize = Sdc::SchemeTypeTraits::GetRegularFaceSize(refiner.GetSchemeType());

    int surfCount = 0;
    for (int face = 0; face < level.getNumFaces(); ++face) {
        int faceSize = level.getNumFaceVertices(face);
        surfCount += faceSize == regFaceSize ? 1 : faceSize;
    }
    return surfCount;
}

inline Sdc::Options& operator << (Sdc::Options& a, Options::FVarBoundary b) {   
    using enum Options::FVarBoundary;
    switch (b) {
        using enum Sdc::Options::FVarLinearInterpolation;
        case kDefault:
            break;
        case kOverride_LinearNone: 
            a.SetFVarLinearInterpolation(FVAR_LINEAR_NONE); break;
        case kOverride_LinearCornersOnly: 
            a.SetFVarLinearInterpolation(FVAR_LINEAR_CORNERS_ONLY); break;
        case kOverride_LinearCornersPlus1: 
            a.SetFVarLinearInterpolation(FVAR_LINEAR_CORNERS_PLUS1); break;
        case kOverride_LinearCornersPlus2: 
            a.SetFVarLinearInterpolation(FVAR_LINEAR_CORNERS_PLUS2); break;
        case kOverride_LinearBoundaries:
            a.SetFVarLinearInterpolation(FVAR_LINEAR_BOUNDARIES); break;
        case kOverride_LinearAll: 
            a.SetFVarLinearInterpolation(FVAR_LINEAR_ALL); break;
        default: assert(0);
    }
    return a;
}


struct RegressionTask::Mesh {
    std::unique_ptr<Far::TopologyRefiner> refiner;
    std::vector<Vec3f> pos;
    std::vector<Vec3f> uvs;
    std::string name;
    fbox3 posbox;
    fbox2 uvbox;
};

std::unique_ptr<RegressionTask::Mesh> RegressionTask::createMesh(ShapeDesc const& shapeDesc) const {

    assert(options);

    auto mesh = std::make_unique<RegressionTask::Mesh>();

    char const* name = shapeDesc.name.data();

    std::unique_ptr<Shape const> shape(Shape::parseObj(shapeDesc.data.data(), shapeDesc.scheme));
    if (!shape) {
        std::fprintf(stderr, "OBJ parsing error - shape %s\n", name);
        return nullptr;
    }
    if (shape->scheme != Scheme::kCatmark && shape->scheme != kLoop) {
        std::fprintf(stderr, "unsupported scheme - shape %s\n", name);
        return nullptr;
    }
    if (shape->uvs.empty() != shape->faceuvs.empty()) {
        std::fprintf(stderr, "incomplete UVs - shape %s\n", name);
        return nullptr;
    }

    if (int numVertices = shape->GetNumVertices(); numVertices > 0) {
        mesh->pos.resize(numVertices);
        std::memcpy(mesh->pos.data(), shape->verts.data(), shape->verts.size() * sizeof(float));
    } else {
        std::fprintf(stderr, "no vertex positions - shape %s\n", name);
        return nullptr;
    }

    if (shape->HasUV()) {

        Vec3f offset = Vec3f{ 0.f, 0.f, shape->bbox.min[2] * 1.25f };

        int numUVs = shape->GetNumUVs();
        mesh->uvs.resize(numUVs);
        for (int i = 0; i < numUVs; ++i) {
            mesh->uvs[i] = offset + Vec3f{ shape->uvs[i * 2], shape->uvs[i * 2 + 1], 0.f };
        }
    }

    {
        Sdc::SchemeType schemeType = GetSdcType(*shape);

        Sdc::Options schemeOptions = GetSdcOptions(*shape);
        schemeOptions << options->fvarBoundary;

        mesh->refiner.reset(Far::TopologyRefinerFactory<Shape>::Create(
            *shape, Far::TopologyRefinerFactory<Shape>::Options(schemeType, schemeOptions)));
    }


    if (!mesh->refiner) {
        std::fprintf(stderr, "unable to create refiner - shape %s\n", name);
        return nullptr;
    }

    mesh->name = name;

    mesh->posbox = shape->bbox;
    mesh->uvbox = fbox2(shape->uvs.data(), (int)shape->uvs.size());

    return mesh;
}

bool RegressionTask::execute() {
    switch (options->mayaLog) {
        using enum Options::MayaLog;
        case kAlways:
            return execute(true);
        case kNever:
            return execute(false);
        case kFailure: {
            // have to run twice :(
            bool status = execute(false);
            if (meshDelta.numFacesWithDeltas > 0) {
                meshDelta = {};
                status &= execute(true);
            }
            return status;
        }
    }
    assert(false);
    return false;
}

bool RegressionTask::execute(bool logMaya) {

    assert(options && shapeDesc);

    MayaLogger logger;   

    if (logMaya)
        logger.Initialize(options->mayaLogPath / shapeDesc->name.data());

    execTime.Start();

    std::unique_ptr<Mesh> mesh = createMesh(*shapeDesc);
    if (!mesh)
        return false;

    Far::TopologyRefiner const& refiner = *mesh->refiner;

    Sdc::SchemeType scheme = refiner.GetSchemeType();

    int regFaceSize = Sdc::SchemeTypeTraits::GetRegularFaceSize(scheme);

    int numFaces = refiner.GetNumFacesTotal();
    //int numSurfaces = countSurfaces(refiner);

    tess::DomainMode domain = scheme == Sdc::SCHEME_CATMARK ?
        tess::DomainMode::QUAD : tess::DomainMode::TRIANGLE;

    auto [patch, patchHalf] = _tessCache.patches(domain);

    farBuildTime.Start();
    auto farEval = std::make_unique<FarEvaluator<float>>(FarEvaluator<float>::Descriptor{
        .options = *options, .baseMesh = *mesh->refiner, .basePos = mesh->pos, .baseUVs = mesh->uvs,});
    farBuildTime.Stop();

    tmrBuildTime.Start();
    auto tmrEval = std::make_unique<TmrEvaluator<float>>(TmrEvaluator<float>::Descriptor{
        .options = *options, .baseMesh = *mesh->refiner, .basePos = mesh->pos, .baseUVs = mesh->uvs, });
    tmrBuildTime.Stop();

    EvalResults<float> farResults;
    EvalResults<float> tmrResults;

    if (!options->ignoreVtx) {
        farResults.evalP        = tmrResults.evalP        = true;
        farResults.eval1stDeriv = tmrResults.eval1stDeriv = options->evaluateD1;
        farResults.eval2ndDeriv = tmrResults.eval2ndDeriv = options->evaluateD2;
    }

    farResults.evalUV = tmrResults.evalUV = options->evaluateUV && !mesh->uvs.empty();

    float pTol = getRelativeTolerance(mesh->posbox, (float)options->tolerance);
    float uvTol = getRelativeTolerance(mesh->uvbox, (float)options->uvTolerance);

    FaceDeltaVectors<float> deltaVecs(pTol, uvTol);

    FaceDelta<float> faceDelta;

    for (int faceIndex = 0, surfIndex = 0; faceIndex < numFaces; ++faceIndex) {

        int faceSize = refiner.getLevel(0).getNumFaceVertices(faceIndex);
        bool isRegular = faceSize == regFaceSize;

        auto evaluate = [&](int surfIndex, tess::Patch const& tessCoords) {

            farEvalTime.Start();
            farEval->Evaluate(surfIndex, tessCoords, farResults);
            farEvalTime.Stop();

            tmrEvalTime.Start();
            tmrEval->Evaluate(surfIndex, tessCoords, tmrResults);
            tmrEvalTime.Stop();

            deltaVecs.pDelta.Compare(farResults.p, tmrResults.p);
            if (options->evaluateD1) {
                deltaVecs.duDelta.Compare(farResults.du, tmrResults.du);
                deltaVecs.dvDelta.Compare(farResults.dv, tmrResults.dv);
            }
            if (options->evaluateD2) {
                deltaVecs.duuDelta.Compare(farResults.duu, tmrResults.duu);
                deltaVecs.duvDelta.Compare(farResults.duv, tmrResults.duv);
                deltaVecs.dvvDelta.Compare(farResults.dvv, tmrResults.dvv);
            }
            if (options->evaluateUV)
                deltaVecs.uvDelta.Compare(farResults.uv, tmrResults.uv);

            faceDelta.AddDeltaVectors(deltaVecs);

            meshDelta.AddFace(faceDelta);

            logger.LogFace(surfIndex, deltaVecs);
        };

        if (farEval->FaceHasLimit(faceIndex)) {

            if (isRegular)
                evaluate(surfIndex, patch);
            else {
                for (int i = 0; i < faceSize; ++i)
                    evaluate(surfIndex + i, patchHalf);
            }
        }
        surfIndex += isRegular ? 1 : faceSize;
    }

    execTime.Stop();
    return true;
}

void RegressionTask::printPass(FILE* f) const {
    std::fprintf(f, "'%s' (%fs): OK \n", shapeDesc->name.data(), execTime.GetTotalElapsedSeconds());
}

void RegressionTask::printMeshDelta(FILE* f) const {

    assert(options);

    if (isKnownFailure)
        std::fprintf(f, "\t*** Passed as known failure ***");
    std::fprintf(f, "\t'%s' (%fs):\n", shapeDesc->name.data(), execTime.GetTotalElapsedSeconds());
    if (meshDelta.numFacesWithPDeltas)
        std::fprintf(f, "\t\tPOS diffs:%6d faces, max delta P  = %g\n",
            meshDelta.numFacesWithPDeltas, (float)meshDelta.maxPDelta);
    if (options->evaluateD1 && meshDelta.numFacesWithD1Deltas)
        std::fprintf(f, "\t\t D1 diffs:%6d faces, max delta D1 = %g\n",
            meshDelta.numFacesWithD1Deltas, (float)meshDelta.maxD1Delta);
    if (options->evaluateD2 && meshDelta.numFacesWithD2Deltas)
        std::fprintf(f, "\t\t D2 diffs:%6d faces, max delta D2 = %g\n",
            meshDelta.numFacesWithD2Deltas, (float)meshDelta.maxD2Delta);
    if (options->evaluateUV && meshDelta.numFacesWithUVDeltas)
        std::fprintf(f, "\t\t UV diffs:%6d faces, max delta UV = %g\n",
            meshDelta.numFacesWithUVDeltas, (float)meshDelta.maxUVDelta);
}

void RegressionTask::printTimes(FILE* f) const {
    std::fprintf(f, "\t'%s':\n", shapeDesc->name.data());
    std::fprintf(f, "\t\tBuild: far:%lf (s) tmr:%lf (\n", 
        farBuildTime.GetTotalElapsedSeconds(), tmrBuildTime.GetTotalElapsedSeconds());
    std::fprintf(f, "\t\tEval: far:%lf (s) tmr:%lf (\n", 
        farEvalTime.GetTotalElapsedSeconds(), tmrEvalTime.GetTotalElapsedSeconds());
    std::fprintf(f, "\t\tExecution:%lf (\n", execTime.GetTotalElapsedSeconds());
}

void RegressionTask::populateTessCache(uint8_t tessRate) {
    tessRate |= 0x1; // even numbers only
    _tessCache.populate(tessRate);
}




