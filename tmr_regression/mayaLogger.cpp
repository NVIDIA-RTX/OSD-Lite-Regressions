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

#include "./mayaLogger.h"

#include <array>
#include <cassert>
#include <concepts>
#include <functional>

namespace fs = std::filesystem;

template <typename REAL> constexpr Vec3<REAL> const green = { .p = {REAL(0), REAL(1), REAL(0)}, };
template <typename REAL> constexpr Vec3<REAL> const red   = { .p = {REAL(1), REAL(0), REAL(0)}, };

template <typename T> char const* vec3Format() {
    static std::array<char const*, 2> const _formats = { "%f %f %f   ", "%lf %lf %lf   ", };
         if constexpr (std::same_as<T, float>) return _formats[0];
    else if constexpr (std::same_as<T, double>) return _formats[1];
    else assert(0);
}

template <int ncols = 5> inline void newLine(FILE* f, int i) {
    if (i > 0 && (((i + 1) % ncols) == 0))
        std::fprintf(f, "\n\t");
}


static void emitMayaPreamble(FILE* f, int mayaVersion = 2020) {
    std::fprintf(f, "//Maya ASCII %d scene\n", mayaVersion);
    std::fprintf(f, "requires maya \"%d\";\n\n", mayaVersion);
    std::fprintf(f, "currentUnit - l centimeter - a degree - t film;\n\n");
}

static void createTransformNode(FILE* f, char const* name, char const* parent = nullptr) {
    if (parent)
        std::fprintf(f, "createNode transform -n \"%s\" -p \"%s\";\n\n", name, parent);
    else
        std::fprintf(f, "createNode transform -n \"%s\";\n\n", name);
}

// note: this emitter is 'legacy' and will be eventually be replaced by 'nParticle' nodes
static void createParticleEmitter(FILE* f, char const* name, char const* parent, bool streaks = false) {
    std::fprintf(f, "createNode particle -n \"%s\"", name);
    if (parent)
        std::fprintf(f, " -p \"%s\"", parent);
    std::fprintf(f, ";\n\n");
    
    if (streaks) {
        std::fprintf(f, "setAttr \".particleRenderType\" 6;\n");

        std::fprintf(f, "addAttr -is true -ci true "
            "-sn \"lineWidth\" -ln \"lineWidth\" -dv 1 -min 1 -max 20 -at \"long\";\n");
        std::fprintf(f, "setAttr -k on \".lineWidth\" 2;\n");

        std::fprintf(f, "addAttr -is true -ci true "
            "-sn \"tailFade\" -ln \"tailFade\" -min -1 -max 1 -at \"float\";\n");
        std::fprintf(f, "setAttr \".tailFade\" 1;\n");

        std::fprintf(f, "addAttr -is true -ci true "
            "-sn \"tailSize\" -ln \"tailSize\" -dv 1 -min -100 -max 100 -at \"float\";\n");
        std::fprintf(f, "setAttr \".tailSize\" 1;\n");
    }
}

template <typename REAL> static void fillVectorAttr(FILE* f,
    char const* attrName, Vec3<REAL> const& value, int nvalues) {

    std::fprintf(f, "setAttr \"%s\" -type \"vectorArray\" %d \n\t", attrName, nvalues);
    for (int i = 0; i < nvalues; ++i) {
        std::fprintf(f, vec3Format<REAL>(), value[0], value[1], value[2]);
        newLine(f, i);
    }
    std::fprintf(f, ";\n\n");
}

template <typename REAL> static void fillVectorAttr(FILE* f,
    char const* attrName, Vec3<REAL> const& value, VectorDelta<REAL> const& predicate) {

    assert(predicate.vectorA && predicate.vectorB && (predicate.vectorA->size() == predicate.vectorB->size()));

    int nvalues = predicate.vectorA->size();    
    std::fprintf(f, "setAttr \"%s\" -type \"vectorArray\" %d \n\t", attrName, predicate.numDeltas);
    for (int i = 0; i < nvalues; ++i) {
        if (predicate.IsInvalid(predicate.Evaluate(i))) {
            std::fprintf(f, vec3Format<REAL>(), value[0], value[1], value[2]);
            newLine(f, i);
        }
    }
    std::fprintf(f, ";\n\n");
}

template <typename REAL> static void setVectorAttr(FILE* f,
    char const* attrName, std::vector<Vec3<REAL>> const& values) {

    int nvalues = (int)values.size();
    std::fprintf(f, "setAttr \"%s\" -type \"vectorArray\" %d \n\t", attrName, nvalues);
    for (int i = 0; i < nvalues; ++i) {
        std::fprintf(f, vec3Format<REAL>(), values[i][0], values[i][1], values[i][2]);
        newLine(f, i);
    }
    std::fprintf(f, ";\n\n");
}

template <typename REAL> static void setVectorAttr(FILE* f,
    char const* attrName, std::vector<Vec3<REAL>> const& values, VectorDelta<REAL> const& predicate) {

    assert(values.size() == predicate.vectorA->size()
        && values.size() == predicate.vectorB->size());

    int nvalues = (int)values.size();

    std::fprintf(f, "setAttr \"%s\" -type \"vectorArray\" %d \n\t", attrName, predicate.numDeltas);
    for (int i = 0; i < nvalues; ++i) {
        if (predicate.IsInvalid(predicate.Evaluate(i))) {
            std::fprintf(f, vec3Format<REAL>(), values[i][0], values[i][1], values[i][2]);
            newLine(f, i);
        }
    }
    std::fprintf(f, ";\n\n");
}

static void setParticleIDs(FILE* f, int nparticles) {
    std::fprintf(f, "setAttr \".id0\" -type \"doubleArray\" %d \n\t", nparticles);
    for (int i = 0; i < nparticles; ++i) {
        std::fprintf(f, "%d ", i);
        newLine<30>(f, i);
    }
    std::fprintf(f, ";\n");
    std::fprintf(f, "setAttr \".nid0\" %d;\n\n", nparticles);
}

template <typename REAL> static void addColorAttr(FILE* f,
    char const* shapePath, char const* shapeName, Vec3<REAL> const& value, int nvalues) {

    std::fprintf(f, "addAttr -s false -ci true -sn \"rgbPP\" -ln \"rgbPP\" -dt \"vectorArray\";\n");
    std::fprintf(f, "addAttr -ci true -h true -sn \"rgbPP0\" -ln \"rgbPP0\" -dt \"vectorArray\";\n");

    fillVectorAttr(f, ".rgbPP0", value, nvalues);

    std::fprintf(f, "connectAttr \"%s|%s.xo[0]\" \"%s|%s.rgbPP\";\n\n",
        shapePath, shapeName, shapePath, shapeName);
}

template<typename REAL> void MayaLogger::logPoints(
    char const* faceName, char const* nodePath, VectorDelta<REAL> const& delta) {

    assert(delta.vectorA && delta.vectorB && (delta.vectorA->size() == delta.vectorB->size()));

    std::array<char, 512> parent;
    std::array<char, 512> name;

    createTransformNode(_handle, faceName, nodePath);

    std::snprintf(parent.data(), parent.size(), "%s|%s", nodePath, faceName);
    std::snprintf(name.data(), name.size(), "%s_Shape", faceName);
    createParticleEmitter(_handle, name.data(), parent.data());

    setParticleIDs(_handle, (int)delta.vectorA->size());
    setVectorAttr(_handle, ".pos0", *delta.vectorA);
    addColorAttr(_handle, nodePath, name.data(), green<REAL>, (int)delta.vectorA->size());

    int ndeltas = logFullBeta ? (int)delta.vectorB->size() :  delta.numDeltas;   
    if (ndeltas > 0) {
        constexpr char const* suffix = logFullBeta ? "beta" : "delta";

        std::snprintf(name.data(), name.size(), "%s_%s_%d", faceName, suffix, ndeltas);
        createTransformNode(_handle, name.data(), nodePath);

        std::snprintf(parent.data(), parent.size(), "%s|%s_%s_%d", nodePath, faceName, suffix, ndeltas);
        std::snprintf(name.data(), name.size(), "%s_%s_%d_Shape", faceName, suffix, ndeltas);
        createParticleEmitter(_handle, name.data(), parent.data());

        setParticleIDs(_handle, ndeltas);

        if constexpr (logFullBeta) {
            setVectorAttr(_handle, ".pos0", *delta.vectorB);
        } else {
            setVectorAttr(_handle, ".pos0", *delta.vectorB, delta);
            std::fprintf(_handle, "addAttr -is true -ln \"pointSize\" -at long -min 1 -max 60 -dv 2;\n");
            std::fprintf(_handle, "setAttr \".pointSize\" 6;\n");
        }
        addColorAttr(_handle, nodePath, name.data(), red<REAL>, ndeltas);
    }
}

template<typename REAL> void MayaLogger::logVecs(char const* faceName,
    char const* nodePath, VectorDelta<REAL> const& pDelta, VectorDelta<REAL> const& d1Delta) {

    assert(pDelta.vectorA && pDelta.vectorB && (pDelta.vectorA->size() == pDelta.vectorB->size()));
    assert(d1Delta.vectorA && d1Delta.vectorB && (d1Delta.vectorA->size() == d1Delta.vectorB->size()));

    std::array<char, 512> parent;
    std::array<char, 512> name;

    createTransformNode(_handle, faceName, nodePath);

    std::snprintf(parent.data(), parent.size(), "%s|%s", nodePath, faceName);
    std::snprintf(name.data(), name.size(), "%s_Shape", faceName);
    createParticleEmitter(_handle, name.data(), parent.data(), true);

    setParticleIDs(_handle, (int)pDelta.vectorA->size());
    setVectorAttr(_handle, ".pos0", *pDelta.vectorA);
    addColorAttr(_handle, nodePath, name.data(), green<REAL>, (int)pDelta.vectorA->size());
    setVectorAttr(_handle, ".vel0", *d1Delta.vectorA);

    if (int ndeltas = d1Delta.numDeltas; ndeltas > 0) {

        std::snprintf(name.data(), name.size(), "%s_deltas_%d", faceName, ndeltas);
        createTransformNode(_handle, name.data(), nodePath);

        std::snprintf(parent.data(), parent.size(), "%s|%s_deltas_%d", nodePath, faceName, ndeltas);
        std::snprintf(name.data(), name.size(), "%s_deltas_%d_Shape", faceName, ndeltas);
        createParticleEmitter(_handle, name.data(), parent.data(), true);

        setParticleIDs(_handle, pDelta.numDeltas);
        setVectorAttr(_handle, ".pos0", *pDelta.vectorB, d1Delta);
        addColorAttr(_handle, nodePath, name.data(), red<REAL>, ndeltas);
        setVectorAttr(_handle, ".vel0", *d1Delta.vectorB, d1Delta);

        std::fprintf(_handle, "setAttr \".lineWidth\" 6;\n");;
    }
}

template<typename REAL> void MayaLogger::LogFace(
    int surfIndex, FaceDeltaVectors<REAL> const& deltaVecs) {

    if (!_handle)
        return;

    assert(deltaVecs.pDelta.vectorA && deltaVecs.pDelta.vectorB);

    char name[512];
    std::snprintf(name, std::size(name), "surf_%04d_deltas_%d", surfIndex, deltaVecs.pDelta.numDeltas);

    std::fprintf(_handle, "\n// %s 8<=========================================\n\n", name);

    // points
    logPoints(name, _mayaDeltaPPath.c_str(), deltaVecs.pDelta);

    // 1st. derivatives
    if (deltaVecs.duDelta.vectorA && deltaVecs.duDelta.vectorB)
        logVecs(name, _mayaDeltaDuPath.c_str(), deltaVecs.pDelta, deltaVecs.duDelta);

    if (deltaVecs.dvDelta.vectorA && deltaVecs.dvDelta.vectorB)
        logVecs(name, _mayaDeltaDvPath.c_str(), deltaVecs.pDelta, deltaVecs.dvDelta);

    // 2nd derivatives
    if (deltaVecs.duuDelta.vectorA && deltaVecs.duuDelta.vectorB)
        logVecs(name, _mayaDeltaDuuPath.c_str(), deltaVecs.pDelta, deltaVecs.duuDelta);

    if (deltaVecs.duvDelta.vectorA && deltaVecs.duvDelta.vectorB)
        logVecs(name, _mayaDeltaDuvPath.c_str(), deltaVecs.pDelta, deltaVecs.duvDelta);

    if (deltaVecs.dvvDelta.vectorA && deltaVecs.dvvDelta.vectorB)
        logVecs(name, _mayaDeltaDvvPath.c_str(), deltaVecs.pDelta, deltaVecs.dvvDelta);

    // UVs
    if (deltaVecs.uvDelta.vectorA && deltaVecs.uvDelta.vectorB)
        logPoints(name, _mayaDeltaUVPath.c_str(), deltaVecs.uvDelta);
}

template void MayaLogger::LogFace(int faceIndex, FaceDeltaVectors<float> const& deltaVecs);
template void MayaLogger::LogFace(int faceIndex, FaceDeltaVectors<double> const& deltaVecs);

void MayaLogger::Initialize(std::filesystem::path const& filepath) {

    assert(!filepath.empty() && filepath.has_filename());

    if (auto parent = filepath.parent_path(); !std::filesystem::is_directory(parent)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(parent, ec)) {
            std::fprintf(stderr, "%s", ec.message().c_str());
            return;
        }            
    }

    _filepath = filepath;
    _filepath.replace_extension(".ma");

    if (_handle = std::fopen(_filepath.generic_string().c_str(), "w"); _handle ) {

        emitMayaPreamble(_handle, 2020);

        std::string rootName = filepath.stem().generic_string();
        assert(!rootName.empty());

        createTransformNode(_handle, rootName.c_str());

        auto createTransform = [this, &rootName](char const* name) {
            createTransformNode(_handle, name, rootName.c_str());
#ifdef _MSC_VER            
            return std::format("{}|{}", rootName, name);
#else            
            return rootName + "|" + name;
#endif            
        };

        _mayaDeltaPPath = createTransform("delta_P");

        _mayaDeltaDuPath = createTransform("delta_dU");
        _mayaDeltaDvPath = createTransform("delta_dV");

        _mayaDeltaDuuPath = createTransform("delta_dUU");
        _mayaDeltaDuvPath = createTransform("delta_dUV");
        _mayaDeltaDvvPath = createTransform("delta_dVV");

        _mayaDeltaUVPath = createTransform("delta_UV");
    }
}

MayaLogger::~MayaLogger() {
    if (_handle)
        std::fclose(_handle);
}