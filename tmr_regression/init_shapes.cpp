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

#include "../shapes/all.h"
#include "./init_shapes.h"

#include <common/shape_utils.h>
#include <common/far_utils.h>

std::array<ShapeDesc, kNumCatmarkShapes + kNumLoopShapes> _shapes = {{

    // Catmark
    { "catmark_cube", catmark_cube.c_str(), kCatmark, false },
    { "catmark_cube_corner0", catmark_cube_corner0.c_str(), kCatmark, false },
    { "catmark_cube_corner1", catmark_cube_corner1.c_str(), kCatmark, false },
    { "catmark_cube_corner2", catmark_cube_corner2.c_str(), kCatmark, false },
    { "catmark_cube_corner3", catmark_cube_corner3.c_str(), kCatmark, false },
    { "catmark_cube_corner4", catmark_cube_corner4.c_str(), kCatmark, false },
    { "catmark_cube_creases0", catmark_cube_creases0.c_str(), kCatmark, false },
    { "catmark_cube_creases1", catmark_cube_creases1.c_str(), kCatmark, false },
    { "catmark_cube_creases2", catmark_cube_creases2.c_str(), kCatmark, false },
    { "catmark_cubes_infsharp", catmark_cubes_infsharp.c_str(), kCatmark, false },
    { "catmark_cubes_semisharp", catmark_cubes_semisharp.c_str(), kCatmark, false },
    { "catmark_dart_edgecorner", catmark_dart_edgecorner.c_str(), kCatmark, false },
    { "catmark_dart_edgeonly", catmark_dart_edgeonly.c_str(), kCatmark, false },
    { "catmark_edgecorner", catmark_edgecorner.c_str(), kCatmark, false },
    { "catmark_edgenone", catmark_edgenone.c_str(), kCatmark, false },
    { "catmark_edgeonly", catmark_edgeonly.c_str(), kCatmark, false },
    { "catmark_fan", catmark_fan.c_str(), kCatmark, false },
    { "catmark_flap", catmark_flap.c_str(), kCatmark, false },
    { "catmark_flap2", catmark_flap2.c_str(), kCatmark, false },
    { "catmark_fvar_bound0", catmark_fvar_bound0.c_str(), kCatmark, false },
    { "catmark_fvar_bound1", catmark_fvar_bound1.c_str(), kCatmark, false },
    { "catmark_fvar_bound2", catmark_fvar_bound2.c_str(), kCatmark, false },
    { "catmark_fvar_bound3", catmark_fvar_bound3.c_str(), kCatmark, false },
    { "catmark_fvar_bound4", catmark_fvar_bound4.c_str(), kCatmark, false },
    { "catmark_fvar_project0", catmark_fvar_project0.c_str(), kCatmark, false },
    { "catmark_chaikin0", catmark_chaikin0.c_str(), kCatmark, false },
    { "catmark_chaikin1", catmark_chaikin1.c_str(), kCatmark, false },
    { "catmark_chaikin2", catmark_chaikin2.c_str(), kCatmark, false },
    { "catmark_gregory_test0", catmark_gregory_test0.c_str(), kCatmark, false },
    { "catmark_gregory_test1", catmark_gregory_test1.c_str(), kCatmark, false },
    { "catmark_gregory_test2", catmark_gregory_test2.c_str(), kCatmark, false },
    { "catmark_gregory_test3", catmark_gregory_test3.c_str(), kCatmark, false },
    { "catmark_gregory_test4", catmark_gregory_test4.c_str(), kCatmark, false },
    { "catmark_gregory_test5", catmark_gregory_test5.c_str(), kCatmark, false },
    { "catmark_gregory_test6", catmark_gregory_test6.c_str(), kCatmark, false },
    { "catmark_gregory_test7", catmark_gregory_test7.c_str(), kCatmark, false },
    { "catmark_gregory_test8", catmark_gregory_test8.c_str(), kCatmark, false },
    { "catmark_hole_test1", catmark_hole_test1.c_str(), kCatmark, false },
    { "catmark_hole_test2", catmark_hole_test2.c_str(), kCatmark, false },
    { "catmark_hole_test3", catmark_hole_test3.c_str(), kCatmark, false },
    { "catmark_hole_test4", catmark_hole_test4.c_str(), kCatmark, false },
    { "catmark_inf_crease0", catmark_inf_crease0.c_str(), kCatmark, false },
    { "catmark_inf_crease1", catmark_inf_crease1.c_str(), kCatmark, false },
    { "catmark_lefthanded", catmark_lefthanded.c_str(), kCatmark, true },
    { "catmark_righthanded", catmark_righthanded.c_str(), kCatmark, false },
    { "catmark_nonman_bareverts", catmark_nonman_bareverts.c_str(), kCatmark, false },
    { "catmark_nonman_edges", catmark_nonman_edges.c_str(), kCatmark, false },
    { "catmark_nonman_edge100", catmark_nonman_edge100.c_str(), kCatmark, false },
    { "catmark_nonman_quadpole8", catmark_nonman_quadpole8.c_str(), kCatmark, false },
    { "catmark_nonman_quadpole64", catmark_nonman_quadpole64.c_str(), kCatmark, false },
    { "catmark_nonman_quadpole360", catmark_nonman_quadpole360.c_str(), kCatmark, false },
    { "catmark_nonman_verts", catmark_nonman_verts.c_str(), kCatmark, false },
    { "catmark_nonquads", catmark_nonquads.c_str(), kCatmark, false },
    { "catmark_pole8", catmark_pole8.c_str(), kCatmark, false },
    { "catmark_pole64", catmark_pole64.c_str(), kCatmark, false },
    { "catmark_pole360", catmark_pole360.c_str(), kCatmark, false },
    { "catmark_pyramid", catmark_pyramid.c_str(), kCatmark, false },
    { "catmark_pyramid_creases0", catmark_pyramid_creases0.c_str(), kCatmark, false },
    { "catmark_pyramid_creases1", catmark_pyramid_creases1.c_str(), kCatmark, false },
    { "catmark_pyramid_creases2", catmark_pyramid_creases2.c_str(), kCatmark, false },
    { "catmark_quadstrips", catmark_quadstrips.c_str(), kCatmark, false },
    { "catmark_single_crease", catmark_single_crease.c_str(), kCatmark, false },
    { "catmark_smoothtris0", catmark_smoothtris0.c_str(), kCatmark, false },
    { "catmark_smoothtris1", catmark_smoothtris1.c_str(), kCatmark, false },
    { "catmark_tent", catmark_tent.c_str(), kCatmark, false },
    { "catmark_tent_creases0", catmark_tent_creases0.c_str(), kCatmark, false },
    { "catmark_tent_creases1", catmark_tent_creases1.c_str(), kCatmark, false },
    { "catmark_toroidal_tet", catmark_toroidal_tet.c_str(), kCatmark, false },
    { "catmark_torus", catmark_torus.c_str(), kCatmark, false },
    { "catmark_torus_creases0", catmark_torus_creases0.c_str(), kCatmark, false },
    { "catmark_torus_creases1", catmark_torus_creases1.c_str(), kCatmark, false },
    { "catmark_val2_interior", catmark_val2_interior.c_str(), kCatmark, false },
    { "catmark_val2_back2back", catmark_val2_back2back.c_str(), kCatmark, false },
    { "catmark_val2_foldover", catmark_val2_foldover.c_str(), kCatmark, false },
    { "catmark_val2_nonman", catmark_val2_nonman.c_str(), kCatmark, false },
    { "catmark_xord_interior", catmark_xord_interior.c_str(), kCatmark, false },
    { "catmark_xord_boundary", catmark_xord_boundary.c_str(), kCatmark, false },
    { "catmark_bishop", catmark_bishop.c_str(), kCatmark, false },
    { "catmark_car", catmark_car.c_str(), kCatmark, false },
    { "catmark_helmet", catmark_helmet.c_str(), kCatmark, false },
    { "catmark_pawn", catmark_pawn.c_str(), kCatmark, false },
    { "catmark_rook", catmark_rook.c_str(), kCatmark, false },

    // Loop

    { "loop_chaikin0", loop_chaikin0.c_str(), kLoop },
    { "loop_chaikin1", loop_chaikin1.c_str(), kLoop },
    { "loop_cube", loop_cube.c_str(), kLoop },
    { "loop_cube_asymmetric", loop_cube_asymmetric.c_str(), kLoop },
    { "loop_cube_creases0", loop_cube_creases0.c_str(), kLoop },
    { "loop_cube_creases1", loop_cube_creases1.c_str(), kLoop },
    { "loop_cubes_semisharp", loop_cubes_semisharp.c_str(), kLoop },
    { "loop_cubes_infsharp", loop_cubes_infsharp.c_str(), kLoop },
    { "loop_fvar_bound0", loop_fvar_bound0.c_str(), kLoop },
    { "loop_fvar_bound1", loop_fvar_bound1.c_str(), kLoop },
    { "loop_fvar_bound2", loop_fvar_bound2.c_str(), kLoop },
    { "loop_fvar_bound3", loop_fvar_bound3.c_str(), kLoop },
    { "loop_icos_semisharp", loop_icos_semisharp.c_str(), kLoop },
    { "loop_icos_infsharp", loop_icos_infsharp.c_str(), kLoop },
    { "loop_icosahedron", loop_icosahedron.c_str(), kLoop },
    { "loop_nonman_edge100", loop_nonman_edge100.c_str(), kLoop },
    { "loop_nonman_verts", loop_nonman_verts.c_str(), kLoop },
    { "loop_nonman_edges", loop_nonman_edges.c_str(), kLoop },
    { "loop_pole360", loop_pole360.c_str(), kLoop },
    { "loop_pole64", loop_pole64.c_str(), kLoop },
    { "loop_pole8", loop_pole8.c_str(), kLoop },
    { "loop_saddle_edgecorner", loop_saddle_edgecorner.c_str(), kLoop },
    { "loop_saddle_edgeonly", loop_saddle_edgeonly.c_str(), kLoop },
    { "loop_tetrahedron", loop_tetrahedron.c_str(), kLoop },
    { "loop_toroidal_tet", loop_toroidal_tet.c_str(), kLoop },
    { "loop_triangle_edgecorner", loop_triangle_edgecorner.c_str(), kLoop },
    { "loop_triangle_edgeonly", loop_triangle_edgeonly.c_str(), kLoop },
    { "loop_triangle_edgenone", loop_triangle_edgenone.c_str(), kLoop },
    { "loop_val2_interior", loop_val2_interior.c_str(), kLoop },
    { "loop_xord_interior", loop_xord_interior.c_str(), kLoop },
    { "loop_xord_boundary", loop_xord_boundary.c_str(), kLoop },
}};

static bool _allInitialized = false;
static std::vector<ShapeDesc> _allShapes;

void initAllShapes() {

    auto const& cshapes = getCatmarkShapes();
    auto const& lshapes = getLoopShapes();

    _allShapes.reserve(cshapes.size() + lshapes.size());
    
    for (auto const& s : cshapes)
        _allShapes.emplace_back(s);
    
    for (auto const& s : lshapes)
        _allShapes.emplace_back(s);

    _allInitialized = true;
}

std::span<ShapeDesc> const getCatmarkShapes() {
    return { _shapes.data(), kNumCatmarkShapes };
}

std::span<ShapeDesc> const getLoopShapes() {
    return { _shapes.data() + kNumCatmarkShapes, kNumLoopShapes };
}

std::span<ShapeDesc> const getAllShapes() {
    return { _shapes.data(), _shapes.size() };
}

std::span<ShapeDesc> const getShapes(Scheme scheme) {

    static std::vector<ShapeDesc> empty;

    switch (scheme) {
        case kCatmark: 
            return getCatmarkShapes();
        case kLoop:
            return getLoopShapes();
        default:
            assert(0);
            return empty;
    }
}

ShapeDesc const* findShape(char const* name) {
    if (auto it = std::find_if(_shapes.begin(), _shapes.end(), [&name](ShapeDesc const& shape) {
        return std::strncmp(shape.name.data(), name, shape.name.size()) == 0; }); it != _shapes.end())
        return &(*it);
    return nullptr;
}
