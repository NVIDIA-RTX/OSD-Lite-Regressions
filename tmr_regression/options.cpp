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

#include "./options.h"
#include "./init_shapes.h"

#include <common/shape_utils.h>

#include <array>
#include <cstring>
#include <ctime>
#include <exception>
#include <numeric>
#include <fstream>
#include <optional>
#include <sstream>

template <typename T> struct EnumArg {
    T value;
    char const* sn = nullptr;
    char const* ln = nullptr;
};

static std::array<EnumArg<Options::ShapeSet>, 4> _shapesDesc = { {
    {Options::ShapeSet::kNone, "none", "no shapes"},
    {Options::ShapeSet::kCatmarkSet, "catmark", "Catmull-Clark shapes"},
    {Options::ShapeSet::kLoopSet, "loop", "Loop shapes"},
    {Options::ShapeSet::kAllSets, "all", "all shapes"},
}};

static std::array<EnumArg<Options::Scheme>, 2> _schemesDesc = {{
    {Options::Scheme::kCatmark, "catmark", "Catmull-Clark"},
    {Options::Scheme::kLoop, "loop", "Loop"},
}};

static std::array<EnumArg<Options::VtxBoundary>, 4> _vtxBoundaryDesc = {{
    {Options::VtxBoundary::kDefault, "default", "default"},
    {Options::VtxBoundary::kOverride_None, "none", "none"},
    {Options::VtxBoundary::kOverride_EdgeOnly, "eonly", "edges only"},
    {Options::VtxBoundary::KOverride_EdgeCorner, "ecorner", "edges & corners"},
}};

static std::array<EnumArg<Options::FVarBoundary>, 7> _fvarBoundaryDesc = {{
    {Options::FVarBoundary::kDefault, "default", "default mode"},
    {Options::FVarBoundary::kOverride_LinearNone, "lnone", "override linear none"},
    {Options::FVarBoundary::kOverride_LinearCornersOnly, "lconly", "override corners only"},
    {Options::FVarBoundary::kOverride_LinearCornersPlus1, "lcpl1", "override corners plus1"},
    {Options::FVarBoundary::kOverride_LinearCornersPlus2, "lcpl2", "override corners plus2"},
    {Options::FVarBoundary::kOverride_LinearBoundaries, "lbnd", "override linear boundaries"},
    {Options::FVarBoundary::kOverride_LinearAll, "lall", "override linear all"},
}};

static std::array<EnumArg<Options::MayaLog>, 3> _mayaLogDesc = {{
    {Options::MayaLog::kNever, "never", "never" },
    {Options::MayaLog::kFailure, "fail", "failure only"},
    {Options::MayaLog::kAlways, "always", "always"},
}};

template <typename T> constexpr char const* operator*(T const& e) {
         if constexpr (std::same_as<Options::ShapeSet, T>) return _shapesDesc[int(e)].sn;
    else if constexpr (std::same_as<Options::Scheme, T>) return _schemesDesc[int(e)].sn;
    else if constexpr (std::same_as<Options::VtxBoundary, T>) return _vtxBoundaryDesc[int(e)].sn;
    else if constexpr (std::same_as<Options::FVarBoundary, T>) return _fvarBoundaryDesc[int(e)].sn;
    else if constexpr (std::same_as<Options::MayaLog, T>) return _mayaLogDesc[int(e)].sn;
}

template <typename T> constexpr char const* operator~(T const& e) {
         if constexpr (std::same_as<Options::ShapeSet, T>) return _shapesDesc[int(e)].ln;
    else if constexpr (std::same_as<Options::Scheme, T>) return _schemesDesc[int(e)].ln;
    else if constexpr (std::same_as<Options::VtxBoundary, T>) return _vtxBoundaryDesc[int(e)].ln;
    else if constexpr (std::same_as<Options::FVarBoundary, T>) return _fvarBoundaryDesc[int(e)].ln;
    else if constexpr (std::same_as<Options::MayaLog, T>) return _mayaLogDesc[int(e)].ln;
}

template <typename T> inline T parseEnum(char const* arg) {

    auto getNumDesc = [&]() {
             if constexpr (std::same_as<T, Options::ShapeSet>) return _shapesDesc;
        else if constexpr (std::same_as<T, Options::Scheme>) return _schemesDesc;
        else if constexpr (std::same_as<T, Options::VtxBoundary>) return _vtxBoundaryDesc;
        else if constexpr (std::same_as<T, Options::FVarBoundary>) return _fvarBoundaryDesc;
        else if constexpr (std::same_as<T, Options::MayaLog>) return _mayaLogDesc;
    };

    auto validArgs = [](auto const& enums) {
        std::string args = " ";
        for (auto e : enums)
            args += std::string(e.sn) + " ";
        return args;
    };

    auto enumDesc = getNumDesc();

    auto it = std::find_if(enumDesc.begin(), enumDesc.end(),
        [&arg](auto item) { return std::strcmp(arg, item.sn) == 0; });

    if (it != enumDesc.end())
        return (*it).value;

    throw std::invalid_argument(
#ifdef _MSC_VER    
        std::format("Error : invalid argument '{}' ; expected [{}]\n", arg, validArgs(enumDesc).c_str()));
#else    
        std::string("Error : invalid argument '") + arg + "' ; expected [" + validArgs(enumDesc) + "]\n");
#endif
}

Scheme convert(Options::Scheme scheme) {
    switch (scheme) {
        case Options::Scheme::kCatmark: return ::Scheme::kCatmark;
        case Options::Scheme::kLoop: return ::Scheme::kLoop;
    }
    assert(0);
    return ::Scheme::kCatmark;
}

std::optional<std::string> readFile(char const* filepath) {

    if (std::ifstream ifs(filepath); ifs.is_open()) {
        std::stringstream ss;
        ss << ifs.rdbuf();
        ifs.close();
        return ss.str();
    }
    return {};
}

void Options::initialize(int argc, char const** argv) {

    static std::vector<std::string> _stringStore;

    char const* shapeName = nullptr;

    for (int i = 1; i < argc; ++i) {

        char const* arg = argv[i];

        if (std::strstr(arg, ".obj")) {

            std::filesystem::path path = arg;

            if (auto obj = readFile(path.lexically_normal().generic_string().c_str())) {
                shapes.push_back(ShapeDesc{
                    .name = path.filename().generic_string(),
                    .data = std::move(*obj),
                    .scheme = convert(scheme),
                    .isLeftHanded = (bool)leftHanded 
                });

                shapeSet = ShapeSet::kNone;
            }
        } else if (!std::strcmp(arg, "-print") || !std::strcmp(arg, "-help") || !std::strcmp(arg, "-?")) {
            throw std::invalid_argument("");
        } else if (!std::strcmp(arg, "-shape")) {
            if (++i < argc) {
                shapeName = argv[i];
                shapeSet = ShapeSet::kNone;
            }
        } else if (!std::strcmp(arg, "-shapeset")) {
            shapeSet = parseEnum<Options::ShapeSet>(++i < argc ? argv[i] : "");
        } else if (!std::strcmp(arg, "-scheme")) {
            scheme = parseEnum<Options::Scheme>(++i < argc ? argv[i] : "");
        } else if (!std::strcmp(arg, "-yup")) {
            leftHanded = true;


        } else if (!std::strcmp(arg, "-isharp")) {
            if (++i < argc) isolationSharp = std::atoi(argv[i]);
        } else if (!std::strcmp(arg, "-ismooth")) {
            if (++i < argc) isolationSmooth = std::atoi(argv[i]);
        } else if (!std::strcmp(arg, "-vtx")) {
            vtxBoundary = parseEnum<Options::VtxBoundary>(++i < argc ? argv[i] : "");
        } else if (!std::strcmp(arg, "-fvar")) {
            fvarBoundary = parseEnum<Options::FVarBoundary>(++i < argc ? argv[i] : "");
        } else if (!std::strcmp(arg, "-tess")) {
            if (++i < argc) tessRate = std::atoi(argv[i]);

        } else if (!std::strcmp(arg, "-tol")) {
            if (++i < argc) tolerance = std::fabs(std::atof(argv[i]));
        } else if (!std::strcmp(arg, "-uvtol")) {
            if (++i < argc) uvTolerance = std::fabs(std::atof(argv[i]));

        } else if (!std::strcmp(arg, "-full")) {
            fullBatchTesting = true;
        } else if (!std::strcmp(arg, "-knownFailures")) {
            ignoreKnownFailures = true;
        } else if (!std::strcmp(arg, "-skipvtx")) {
            ignoreVtx = true;
        } else if (!std::strcmp(arg, "-d1")) {
            evaluateD1 = true;
        } else if (!std::strcmp(arg, "-nod1")) {
            evaluateD1 = false;
        } else if (!std::strcmp(arg, "-d2")) {
            evaluateD2 = true;
        } else if (!std::strcmp(arg, "-nod2")) {
            evaluateD2 = false;
        } else if (!std::strcmp(arg, "-uv")) {
            evaluateUV = true;
        } else if (!std::strcmp(arg, "-nouv")) {
            evaluateD2 = false;

        } else if (!std::strcmp(arg, "-mt")) {
            multi_threaded = true;
        } else if (!std::strcmp(arg, "-nomt")) {
            multi_threaded = false;

        } else if (!std::strcmp(arg, "-prog")) {
            printProgress = true;
        } else if (!std::strcmp(arg, "-noprog")) {
            printProgress = false;
        } else if (!std::strcmp(arg, "-sum")) {
            printSummary = true;
        } else if (!std::strcmp(arg, "-nosum")) {
            printSummary = false;

        } else if (!std::strcmp(arg, "-mayalog")) {
            mayaLog = parseEnum<Options::MayaLog>(++i < argc ? argv[i] : "");
        } else if (!std::strcmp(arg, "-mayapath")) {
            if (++i < argc) mayaLogPath = argv[i];
            if (!std::filesystem::is_directory(mayaLogPath)) {
                throw std::invalid_argument(                
#ifdef _MSC_VER
                    std::format("Error: mayaLogpath is not a directory '{}'", mayaLogPath.generic_string()));
#else
                    std::string("Error: mayaLogpath is not a directory '") + mayaLogPath.generic_string() + "'");
#endif                    
            }
        } else if (!std::strcmp(arg, "-statspath")) {
            if (++i < argc) statisticsFilePath = argv[i];
        } else {
#ifdef _MSC_VER            
            throw std::invalid_argument(std::format("Error: unknown argument '{}'", argv[i]));
#else
            throw std::invalid_argument(std::string("Error: unknown argument '") + argv[i] + "'");
#endif            
        }
    }

    if (ignoreVtx)
        evaluateD1 = evaluateD2 = false;

    if (evaluateD2 && !evaluateD1) {
        std::fprintf(stderr, "Warning: 2nd deriv evaluation forces 1st.\n");
        evaluateD1 = true;
    }

    if (evaluateUV && !(ignoreVtx || (fvarBoundary == FVarBoundary::kOverride_LinearAll))) {
        std::fprintf(stderr, "Warning: simultaneous evaluation of cubic vertex & face-varying limits is unstable.\n");
        std::fprintf(stderr, "         Ignore vertex results (-skiptvtx) or override fvar (-fvar lall).\n");
    }

    if (ignoreVtx && !evaluateUV)
        throw std::invalid_argument("Error: nothing to evaluate (-skipvtx and -nouv).\n");


    if (tessRate > std::numeric_limits<uint8_t>::max()) {
        std::fprintf(stderr, "Warning: max tessellation rate is 255.\n");
        tessRate = std::numeric_limits<uint8_t>::max();
    }

    if ((isolationSharp == 0) || (isolationSmooth == 0)) {
        std::fprintf(stderr, "Warning: unstable evaluation with isolation level 0.\n");
    }

    if (shapeName) {
        if (ShapeDesc const* shape = findShape(shapeName))
            shapes.push_back(*shape);
    }

    if (shapes.empty()) {
        std::span<ShapeDesc> s;
        switch (shapeSet) {
            case ShapeSet::kCatmarkSet: s = getCatmarkShapes(); break;
            case ShapeSet::kLoopSet: s = getLoopShapes(); break;
            case ShapeSet::kAllSets: s = getAllShapes(); break;
            default: assert(0);
        }
        if (!s.empty())
            shapes.assign(s.begin(), s.end());
    }

    if (shapes.empty())
        throw std::invalid_argument("Error: no shape to test");
}

inline char const* str(bool b) { return b ? "true" : "false"; }

void Options::print(FILE *f, PrintMask mask) const {

    if (uint8_t(mask) & uint8_t(PrintMask::kGeneralInfo)) {
        std::fprintf(f, "\tGeneral Info:\n");

        char buf[100];
        if (std::strftime(buf, sizeof(buf), "%c %Z", std::localtime(&timeStamp)))
            std::fprintf(f, "\t\t -date                                  = %s\n", buf);

        std::fprintf(f, "\t\t -full          (full test batching)    = %s\n", str(fullBatchTesting));
        std::fprintf(f, "\t\t -knownFailures (ignore known failures) = %s\n", str(ignoreKnownFailures));
        if (shapeSet == ShapeSet::kNone) {
            for (auto shape : shapes)
                std::fprintf(f, "\t\t -shape                             = '%s'\n", shape.name.data());
        } else {
            std::fprintf(f, "\t\t -shapeset      (set of shapes)         = '%s'\n", ~shapeSet);
        }
        std::fprintf(f, "\t\t -scheme        (OBJ shapes)            = '%s'\n", ~scheme);
        std::fprintf(f, "\t\t -tess          (tessellation rate)     = %d\n", tessRate);
        std::fprintf(f, "\t\t -isharp        (isolation sharp)       = %d\n", isolationSharp);
        std::fprintf(f, "\t\t -ismooth       (isolation smooth)      = %d\n", isolationSmooth);
    }
    if (uint8_t(mask) & uint8_t(PrintMask::kComparisonOptions)) {
        std::fprintf(f, "\t\t                (double precision)      = %s\n", str(doublePrecision));
        std::fprintf(f, "\t\t -tol           (abs tolerance)         = %g\n", tolerance);
        std::fprintf(f, "\t\t -uvtol         (abs uv tolerance)      = %g\n", uvTolerance);
    }
    if (uint8_t(mask) & uint8_t(PrintMask::kEvaluationOptions)) {
        std::fprintf(f, "\tEvaluation Options:\n");
        std::fprintf(f, "\t\t -vtx           (vertex bnd interp)     = '%s'\n", ~vtxBoundary);
        std::fprintf(f, "\t\t -fvar          (fvar bnd interp)       = '%s'\n", ~fvarBoundary);
        std::fprintf(f, "\t\t -skipvtx       (ignore vertex eval)    = %s\n", str(ignoreVtx));
        std::fprintf(f, "\t\t -d1            (eval 1st deriv)        = %s\n", str(evaluateD1));
        std::fprintf(f, "\t\t -d2            (eval 2nd deriv)        = %s\n", str(evaluateD2));
        std::fprintf(f, "\t\t -uv            (eval UVs)              = %s\n", str(evaluateUV));
    }

    if (uint8_t(mask) & uint8_t(PrintMask::kOutputOptions)) {
        std::fprintf(f, "\tOutput Options:\n");
        std::fprintf(f, "\t\t -mayalog       (maya log mode)         = '%s'\n", ~mayaLog);
        std::fprintf(f, "\t\t -mayapath      (maya files path)       = '%s'\n", mayaLogPath.lexically_normal().generic_string().c_str());
        std::fprintf(f, "\t\t -statspath     (stats files path)      = '%s'\n", statisticsFilePath.lexically_normal().generic_string().c_str());
    }
}
