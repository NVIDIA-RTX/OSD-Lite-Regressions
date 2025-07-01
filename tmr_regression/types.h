//
//   Copyright 2021 Pixar
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
#include <cmath>
#include <cstring>
#include <vector>

//
//  Simple interpolatable struct for (x,y,z) positions and normals:
//
template <typename REAL> struct Vec3 {

    //  Clear() and AddWithWeight() required for interpolation:
    void Clear( void * =0 ) { p[0] = p[1] = p[2] = 0.0f; }

    void Set(Vec3<REAL> const& src, REAL weight) {
        p[0] = weight * src.p[0];
        p[1] = weight * src.p[1];
        p[2] = weight * src.p[2];
    }

    void AddWithWeight(Vec3<REAL> const & src, REAL weight) {
        p[0] += weight * src.p[0];
        p[1] += weight * src.p[1];
        p[2] += weight * src.p[2];
    }

    //  Element access via []:
    REAL const & operator[](int i) const { return p[i]; }
    REAL       & operator[](int i)       { return p[i]; }

    //  Element access via []:
    REAL const * Coords() const { return p; }
    REAL       * Coords()       { return p; }

    //  Additional useful mathematical operations:
    Vec3<REAL> operator-() const {
        return Vec3<REAL>{ -p[0], -p[1], -p[2] };
    }
    Vec3<REAL> operator-(Vec3<REAL> const & x) const {
        return Vec3<REAL>{p[0] - x.p[0], p[1] - x.p[1], p[2] - x.p[2]};
    }
    Vec3<REAL> operator+(Vec3<REAL> const & x) const {
        return Vec3<REAL>{p[0] + x.p[0], p[1] + x.p[1], p[2] + x.p[2]};
    }
    Vec3<REAL> operator*(REAL s) const {
        return Vec3<REAL>{p[0] * s, p[1] * s, p[2] * s};
    }
    Vec3<REAL> Cross(Vec3<REAL> const & x) const {
        return Vec3<REAL>{p[1]*x.p[2] - p[2]*x.p[1],
                          p[2]*x.p[0] - p[0]*x.p[2],
                          p[0]*x.p[1] - p[1]*x.p[0]};
    }
    REAL Dot(Vec3<REAL> const & x) const {
        return p[0]*x.p[0] + p[1]*x.p[1] + p[2]*x.p[2];
    }
    REAL Length() const {
        return std::sqrt(this->Dot(*this));
    }

    //  Static method to compute normal vector:
    static Vec3<REAL> ComputeNormal(Vec3<REAL> const & Du, Vec3<REAL> const & Dv,
                             REAL eps = 0.0f) {
        Vec3<REAL> N = Du.Cross(Dv);
        REAL lenSqrd = N.Dot(N);
        if (lenSqrd <= eps) return Vec3<REAL>(0.0f, 0.0f, 0.0f);
        return N * (1.0f / std::sqrt(lenSqrd));
    }

    //  Member variables (XYZ coordinates):
    REAL p[3];
};

typedef Vec3<float>  Vec3f;
typedef Vec3<double> Vec3d;


//
//  Simple struct to hold the results of a face evaluation:
//
template <typename REAL> struct EvalResults {

    bool evalP = false;
    bool eval1stDeriv = false;
    bool eval2ndDeriv = false;
    bool evalUV = false;

    std::vector<Vec3<REAL>> p;
    std::vector<Vec3<REAL>> du;
    std::vector<Vec3<REAL>> dv;
    std::vector<Vec3<REAL>> duu;
    std::vector<Vec3<REAL>> duv;
    std::vector<Vec3<REAL>> dvv;

    std::vector<Vec3<REAL>> uv;

    void Resize(int size) {
        if (evalP) {
            p.resize(size);
            if (eval1stDeriv) {
                du.resize(size);
                dv.resize(size);
                if (eval2ndDeriv) {
                    duu.resize(size);
                    duv.resize(size);
                    dvv.resize(size);
                }
            }
        }
        if (evalUV)
            uv.resize(size);
    }
};


//
//  Simple struct to hold the differences between two vectors:
//
template <typename REAL> class VectorDelta {
public:
    typedef std::vector< Vec3<REAL> >  VectorVec3;

public:

    std::vector<Vec3<REAL>> const * vectorA = nullptr;
    std::vector<Vec3<REAL>> const * vectorB = nullptr;

    int  numDeltas  = 0;
    REAL maxDelta  = 0.f;
    REAL tolerance  = 0.f;

public:
    VectorDelta(REAL epsilon = 0.0f) : tolerance(epsilon) { }

    inline Vec3<REAL> Evaluate(int index) const {
        Vec3<REAL> const& a = (*vectorA)[index];
        Vec3<REAL> const& b = (*vectorB)[index];
        return { std::abs(a[0] - b[0]),
                 std::abs(a[1] - b[1]),
                 std::abs(a[2] - b[2]), };
    }


    inline bool IsInvalid(Vec3<REAL> delta) const {
        return (delta[0] > tolerance) || (delta[1] > tolerance) || (delta[2] > tolerance);
    }

    void Compare(VectorVec3 const & a, VectorVec3 const & b) {

        assert(a.size() == b.size());

        vectorA = &a;
        vectorB = &b;

        numDeltas = 0;
        maxDelta = 0.0f;

        for (int i = 0; i < (int)a.size(); ++i) {           
            if (Vec3<REAL> d = Evaluate(i); IsInvalid(d)) {
                ++ numDeltas;
                if (maxDelta < d[0]) maxDelta = d[0];
                if (maxDelta < d[1]) maxDelta = d[1];
                if (maxDelta < d[2]) maxDelta = d[2];
            }
        }
    }
};

template <typename REAL> struct FaceDeltaVectors {

    FaceDeltaVectors(REAL tol, REAL uvtol) {
        float pTol = tol;
        float d1Tol = pTol * 5.f;
        float d2Tol = d1Tol * 5.f;

        pDelta.tolerance = pTol;
        duDelta.tolerance = d1Tol;
        dvDelta.tolerance = d1Tol;
        duuDelta.tolerance = d2Tol;
        duvDelta.tolerance = d2Tol;
        dvvDelta.tolerance = d2Tol;

        uvDelta.tolerance = uvtol;
    }

    VectorDelta<REAL> pDelta;
    VectorDelta<REAL> duDelta;
    VectorDelta<REAL> dvDelta;
    VectorDelta<REAL> duuDelta;
    VectorDelta<REAL> duvDelta;
    VectorDelta<REAL> dvvDelta;
    VectorDelta<REAL> uvDelta;
};

template <typename REAL> class FaceDelta {
public:
    //  Member variables:
    bool hasDeltas;
    bool hasGeomDeltas;
    bool hasUVDeltas;

    int numPDeltas;
    int numD1Deltas;
    int numD2Deltas;
    int numUVDeltas;

    REAL maxPDelta;
    REAL maxD1Delta;
    REAL maxD2Delta;
    REAL maxUVDelta;

public:
    FaceDelta() { Clear(); }

    void Clear() {
        std::memset(this, 0, sizeof(*this));
    }

    void AddPDelta(VectorDelta<REAL> const & pDelta) {
        if (pDelta.numDeltas) {
            numPDeltas = pDelta.numDeltas;
            maxPDelta  = pDelta.maxDelta;
            hasDeltas = hasGeomDeltas = true;
        }
    }
    void AddDuDelta(VectorDelta<REAL> const & duDelta) {
        if (duDelta.numDeltas) {
            numD1Deltas += duDelta.numDeltas;
            maxD1Delta   = std::max(maxD1Delta, duDelta.maxDelta);
            hasDeltas = hasGeomDeltas = true;
        }
    }
    void AddDvDelta(VectorDelta<REAL> const & dvDelta) {
        if (dvDelta.numDeltas) {
            numD1Deltas += dvDelta.numDeltas;
            maxD1Delta   = std::max(maxD1Delta, dvDelta.maxDelta);
            hasDeltas = hasGeomDeltas = true;
        }
    }
    void AddDuuDelta(VectorDelta<REAL> const & duuDelta) {
        if (duuDelta.numDeltas) {
            numD2Deltas += duuDelta.numDeltas;
            maxD2Delta   = std::max(maxD2Delta, duuDelta.maxDelta);
            hasDeltas = hasGeomDeltas = true;
        }
    }
    void AddDuvDelta(VectorDelta<REAL> const & duvDelta) {
        if (duvDelta.numDeltas) {
            numD2Deltas += duvDelta.numDeltas;
            maxD2Delta   = std::max(maxD2Delta, duvDelta.maxDelta);
            hasDeltas = hasGeomDeltas = true;
        }
    }
    void AddDvvDelta(VectorDelta<REAL> const & dvvDelta) {
        if (dvvDelta.numDeltas) {
            numD2Deltas += dvvDelta.numDeltas;
            maxD2Delta   = std::max(maxD2Delta, dvvDelta.maxDelta);
            hasDeltas = hasGeomDeltas = true;
        }
    }
    void AddUVDelta(VectorDelta<REAL> const & uvDelta) {
        if (uvDelta.numDeltas) {
            numUVDeltas = uvDelta.numDeltas;
            maxUVDelta  = uvDelta.maxDelta;
            hasDeltas = hasUVDeltas = true;
        }
    }

    void AddDeltaVectors(FaceDeltaVectors<REAL> const& deltas) {
        Clear();
        AddPDelta(deltas.pDelta);
        AddDuDelta(deltas.duDelta);
        AddDvDelta(deltas.dvDelta);
        AddDuuDelta(deltas.duuDelta);
        AddDuvDelta(deltas.duvDelta);
        AddDvvDelta(deltas.dvvDelta);
        AddUVDelta(deltas.uvDelta);
    }
};

template <typename REAL> class MeshDelta {
public:
    //  Member variables:
    int numFacesWithDeltas = 0;
    int numFacesWithGeomDeltas = 0;
    int numFacesWithUVDeltas = 0;

    int numFacesWithPDeltas = 0;
    int numFacesWithD1Deltas = 0;
    int numFacesWithD2Deltas = 0;

    REAL maxPDelta = REAL(0);
    REAL maxD1Delta = REAL(0);
    REAL maxD2Delta = REAL(0);
    REAL maxUVDelta = REAL(0);

public:

    void AddFace(FaceDelta<REAL> const & faceDelta) {

        numFacesWithDeltas     += faceDelta.hasDeltas;
        numFacesWithGeomDeltas += faceDelta.hasGeomDeltas;
        numFacesWithUVDeltas   += faceDelta.hasUVDeltas;

        numFacesWithPDeltas  += (faceDelta.numPDeltas  > 0);
        numFacesWithD1Deltas += (faceDelta.numD1Deltas > 0);
        numFacesWithD2Deltas += (faceDelta.numD2Deltas > 0);

        maxPDelta  = std::max(maxPDelta,  faceDelta.maxPDelta);
        maxD1Delta = std::max(maxD1Delta, faceDelta.maxD1Delta);
        maxD2Delta = std::max(maxD2Delta, faceDelta.maxD2Delta);
        maxUVDelta = std::max(maxUVDelta, faceDelta.maxUVDelta);
    }
};
