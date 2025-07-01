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

#include "./tess.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>

using namespace std;

namespace tess {

// uniform tessellation

inline void output_tri(std::vector<int>& indices, int idx0, int idx1, int idx2) {
    indices.push_back(idx0);
    indices.push_back(idx1);
    indices.push_back(idx2);
};

namespace uniform {

std::pair<int, int> patchSize(DomainMode domain, int lod) {

    int ntriangles = 0;
    int nverts = 0;

    switch (domain) {
        case DomainMode::QUAD: {
            ntriangles = lod > 1 ? (2 * (lod - 1) * (lod - 1)) : 2;
            nverts = lod > 1 ? (lod * lod) : 4;
        } break;
        case DomainMode::TRIANGLE: {
            int nedgeverts = lod + 2;
            nverts = (nedgeverts * (nedgeverts + 1)) / 2;
            ntriangles = (lod + 1) * (lod + 1);
        } break;
        default:
            assert(0);
    }
    return { ntriangles, nverts };
}

void tessellate(DomainMode domain, int lod, Patch& patch) {

    auto& indices = patch.indices;
    auto& u = patch.u;
    auto& v = patch.v;

    if (domain == DomainMode::QUAD) {
        if (lod <= 1) {
            indices.clear();
            indices.reserve(6);
            output_tri(indices, 0, 2, 1);
            output_tri(indices, 0, 3, 2);
            u.resize(4);
            v.resize(4);
            u[0] = 0.0f; v[0] = 0.0f;
            u[1] = 0.0f; v[1] = 1.0f;
            u[2] = 1.0f; v[2] = 1.0f;
            u[3] = 1.0f; v[3] = 0.0f;
        } else {
            int nverts = lod * lod;
            int ntriangles = 2 * (lod - 1) * (lod - 1);

            indices.clear();
            indices.reserve(ntriangles * 3);
            for (int i = 0; i < (lod - 1); ++i) {
                for (int j = 0; j < (lod - 1); ++j) {

                    int idx0 = j * lod + i,
                        idx1 = idx0 + lod,
                        idx2 = idx1 + 1,
                        idx3 = idx0 + 1;

                    output_tri(indices, idx0, idx2, idx1);
                    output_tri(indices, idx0, idx3, idx2);
                }
            }

            u.resize(nverts);
            v.resize(nverts);
            for (int j = 0; j < lod; ++j) {
                for (int i = 0; i < lod; ++i) {
                    int index = j * lod + i;
                    u[index] = (float)i / ((float)lod - 1.0f);
                    v[index] = (float)j / ((float)lod - 1.0f);
                }
            }
        }
    } else if (domain == DomainMode::TRIANGLE) {
        if (lod < 1) {
            indices.clear();
            indices.reserve(3);
            output_tri(indices, 0, 1, 2);
            u.resize(3);
            v.resize(3);
            u[0] = 0.0f; v[0] = 0.0f;
            u[1] = 0.0f; v[1] = 1.0f;
            u[2] = 1.0f; v[2] = 0.0f;
        } else {
            int nedgeverts = lod + 2;
            int nverts = (nedgeverts * (nedgeverts + 1)) / 2;
            int ntriangles = (lod + 1) * (lod + 1);

            indices.clear();
            indices.reserve(ntriangles * 3);
            for (int i = 0, nrows = nedgeverts, ncols = nrows, idxbase = 0; i < (nrows-1); ++i, --ncols) {
                for (int j = 0; j < (ncols-1); ++j) {
                    int idx0 = idxbase + j;
                    int idx1 = idx0 + ncols;
                    int idx2 = idx0 + 1;
                    int idx3 = idx1 + 1;
                    output_tri(indices, idx0, idx1, idx2);
                    if (j < (ncols - 2))
                        output_tri(indices, idx2, idx1, idx3);
                }
                idxbase += ncols;
            }

            u.resize(nverts);
            v.resize(nverts);
            int nrows = nedgeverts;
            int ncols = nedgeverts;            
            for (int i = 0, index = 0; i < nrows; ++i, --ncols) {
                float s = float(i) / float(nrows - 1);
                for (int j = 0; j < ncols; ++j, ++index) {
                    float t = ncols == 1 ? 0.f : float(j) / float(ncols - 1);
                    u[index] = s;
                    v[index] = t * (1.f - s);
                }
            }
        }
    }
}

} // end namespace uniform
} // end namespace tess

