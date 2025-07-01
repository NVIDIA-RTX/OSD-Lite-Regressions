//
//   Copyright 2024 Nvidia
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

#include <cassert>
#include <cstdint>
#include <vector>

namespace tess {

    enum class DomainMode : uint8_t {
        ISOLINE = 0,
        TRIANGLE,
        QUAD
    };

    template <DomainMode> inline std::pair<float, float> rotateDomain(uint8_t rot, float u, float v);
    template <DomainMode> inline std::pair<float, float> rotateDomainInv(uint8_t rot, float u, float v);

    inline std::pair<float, float> rotateDomain(DomainMode domain, int rot, float u, float v);
    inline std::pair<float, float> rotateDomainInv(DomainMode domain, int rot, float u, float v);

    enum class SpacingMode : uint8_t {
        FRACTIONAL_ODD = 0,
        FRACTIONAL_EVEN = 1,
        EQUAL = 2
    };

    struct Patch {

        int numTriangles() const { return (int)indices.size() / 3; }

        int numVertices() const { return (int)u.size(); }

        void clear() { indices.clear(); u.clear(); v.clear(); }

        std::vector<int> indices;
        std::vector<float> u, v;
    };

    namespace uniform {

        // returns the number of triangles & vertices of a patch
        std::pair<int, int> patchSize(DomainMode domain, int lod);

        // tessellates a patch
        void tessellate(DomainMode domain, int outer, Patch& patch);
    }

    namespace spaced {

        // returns the number of triangles & vertices of a patch
        std::pair<int, int> patchSize(DomainMode domain,
            SpacingMode spacing, float const* inner, float const* outer);

        // tessellates a patch
        void tessellate(DomainMode domain, SpacingMode spacing,
            float const * inner, float const * outer, Patch& patch);
    }

    // implementation

    template <> inline std::pair<float, float> rotateDomain<DomainMode::QUAD>(uint8_t rot, float u, float v) {
        switch (rot) {
            case 0: return {       u,       v };
            case 1: return { 1.f - v,       u };
            case 2: return { 1.f - u, 1.f - v };
            case 3: return {       v, 1.f - u };
        }
        assert(0);
        return {};
    }

    template <> inline std::pair<float, float> rotateDomainInv<DomainMode::QUAD>(uint8_t rot, float u, float v) {
        switch (rot) {
            case 0: return {       u,       v };
            case 1: return {       v, 1.f - u };
            case 2: return { 1.f - u, 1.f - v };
            case 3: return { 1.f - v,       u };
        }
        assert(0);
        return {};
    }

    template <> inline std::pair<float, float> rotateDomain<DomainMode::TRIANGLE>(uint8_t rot, float u, float v) {
        switch (rot) {
            case 0: return {           u,            v };
            case 2: return {           v,  1.f - u - v };
            case 1: return { 1.f - u - v,            u };
        }
        assert(0);
        return {};
    }

    template <> inline std::pair<float, float> rotateDomainInv<DomainMode::TRIANGLE>(uint8_t rot, float u, float v) {
        switch (rot) {
            case 0: return {           u,            v };
            case 2: return { 1.f - u - v,            u };
            case 1: return {           v,  1.f - u - v };
        }
        assert(0);
        return {};
    }

    inline std::pair<float, float> rotateDomain(DomainMode domain, int rot, float u, float v) {
        switch (domain) {
            case DomainMode::QUAD: return rotateDomain<DomainMode::QUAD>(rot, u, v);
            case DomainMode::TRIANGLE: return  rotateDomain<DomainMode::TRIANGLE>(rot, u, v);
            default: assert(0);
        }
        return {};
    }

    inline std::pair<float, float> rotateDomainInv(DomainMode domain, int rot, float u, float v) {
        switch (domain) {
            case DomainMode::QUAD: return rotateDomainInv<DomainMode::QUAD>(rot, u, v);
            case DomainMode::TRIANGLE: return  rotateDomainInv<DomainMode::TRIANGLE>(rot, u, v);
            default: assert(0);
        }
        return {};
    }

} // end namespace tess
