//
//   Copyright 2015 Nvidia
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

#include <algorithm>
#include <array>
#include <limits>
#include <cstdint>

template <typename T, int n> struct box {

    typedef std::array<T, n> value_type;

    static constexpr float const tmin = std::numeric_limits<T>::lowest();
    static constexpr float const tmax = std::numeric_limits<T>::max();

    value_type min;
    value_type max;

    box() {
        min.fill(tmax);
        max.fill(tmin);
    }

    box(value_type const& imin, value_type const& imax) {
        min = imin;
        max = imax;
    }

    box(value_type const* points, int npoints) {
        if (npoints == 0) {
            min.fill(tmax);
            max.fill(tmin);
        } else {
            min = points[0];
            max = points[0];
            for (int i = 1; i < npoints; ++i) {
                for (int j = 0; j < n; ++j) {
                    value_type const& point = points[i];
                    min[j] = std::min(min[j], point[j]);
                    max[j] = std::max(max[j], point[j]);
                }
            }
        }
    }

    box(T const* values, int nvalues) {
        *this = box(reinterpret_cast<value_type const*>(values), nvalues / n);
    }

    inline bool empty() const {
        for (uint8_t i = 0; i < n; ++i)
            if (min[i] > max[i])
                return false;
        return true;
    }

    inline value_type center() const {
        value_type center;
        for (uint8_t i = 0; i < n; ++i)
            center[i] = .5f * (min[i] + max[i]);
        return center;
    }

    inline value_type diagonal() const {
        value_type diagonal;
        for (uint8_t i = 0; i < n; ++i)
            diagonal[i] = (max[i] - min[i]);
        return diagonal;
    }

    inline bool contains(value_type const& p) const {
        for (uint8_t i = 0; i < n; ++i)
            if (min[i] > p[i] || p[i] > max[i])
                return false;
        return true;
    }

    inline void grow(value_type const& a) {
        for (uint8_t i = 0; i < n; ++i) {
            min[i] = std::min(min[i], a[i]);
            max[i] = std::max(max[i], a[i]);
        }
    }

    inline void grow(box<T, n> const& b) {
        for (uint8_t i = 0; i < n; ++i) {
            min[i] = std::min(min[i], b.min[i]);
            max[i] = std::max(max[i], b.max[i]);
        }
    }

    inline value_type clamp(value_type const& a) {
        value_type b;
        for (uint8_t i = 0; i < n; ++i)
            b[i] = std::clamp(a, min[i], max[i]);
        return b;
    }
};

typedef box<int, 2> ibox2;
typedef box<int, 3> ibox3;
typedef box<int, 4> ibox4;

typedef box<float, 2> fbox2;
typedef box<float, 3> fbox3;
typedef box<float, 4> fbox4;
