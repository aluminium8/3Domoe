#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include "MITSUDomoe/3D_objects.hpp"
#include <rfl.hpp>
#include <rfl_eigen_serdes.hpp>
#include <igl/doublearea.h>
#include <igl/barycenter.h>
#include <numeric>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

class CutMeshCartridge
{
public:
    struct Input
    {
        rfl::Field<"input_mesh", MITSU_Domoe::Polygon_mesh> input_mesh;
    };

    struct Output
    {
        rfl::Field<"mesh_a", MITSU_Domoe::Polygon_mesh> mesh_a;
        rfl::Field<"mesh_b", MITSU_Domoe::Polygon_mesh> mesh_b;
        rfl::Field<"message", std::string> message;
    };

    static inline const std::string command_name = "cutMeshByArea";
    static inline const std::string description = "Cuts a mesh into two halves of approximately equal surface area with a plane perpendicular to the X-axis.";

    Output execute(const Input &input) const
    {
        const auto& V = input.input_mesh.value().V;
        const auto& F = input.input_mesh.value().F;

        if (V.rows() == 0 || F.rows() == 0) {
            return Output{ .mesh_a = {}, .mesh_b = {}, .message = "Input mesh is empty." };
        }

        // 1. Calculate face areas and centroids
        Eigen::VectorXd face_areas;
        igl::doublearea(V, F, face_areas);
        face_areas /= 2.0;
        const double total_area = face_areas.sum();

        Eigen::MatrixXd face_centroids;
        igl::barycenter(V, F, face_centroids);

        // 2. Find the median x-coordinate
        std::vector<int> face_indices(F.rows());
        std::iota(face_indices.begin(), face_indices.end(), 0);
        std::sort(face_indices.begin(), face_indices.end(),
                  [&](int a, int b) { return face_centroids(a, 0) < face_centroids(b, 0); });

        double accumulated_area = 0.0;
        double median_x = 0.0;
        int split_idx = 0;
        for (int i = 0; i < F.rows(); ++i) {
            int face_idx = face_indices[i];
            accumulated_area += face_areas(face_idx);
            if (accumulated_area >= total_area / 2.0) {
                median_x = face_centroids(face_idx, 0);
                split_idx = i;
                break;
            }
        }

        // 3. Split faces into two groups
        std::vector<int> faces_a_indices, faces_b_indices;
        for(int i = 0; i < F.rows(); ++i) {
            if (i <= split_idx) {
                faces_a_indices.push_back(face_indices[i]);
            } else {
                faces_b_indices.push_back(face_indices[i]);
            }
        }

        auto create_submesh = [&](const std::vector<int>& selected_face_indices) {
            MITSU_Domoe::Polygon_mesh submesh;
            if (selected_face_indices.empty()) return submesh;

            std::set<int> unique_vert_indices;
            for (int face_idx : selected_face_indices) {
                for (int i = 0; i < F.cols(); ++i) {
                    unique_vert_indices.insert(F(face_idx, i));
                }
            }

            std::map<int, int> old_to_new_vert_map;
            submesh.V.resize(unique_vert_indices.size(), V.cols());
            int new_idx = 0;
            for (int old_idx : unique_vert_indices) {
                submesh.V.row(new_idx) = V.row(old_idx);
                old_to_new_vert_map[old_idx] = new_idx;
                new_idx++;
            }

            submesh.F.resize(selected_face_indices.size(), F.cols());
            for (size_t i = 0; i < selected_face_indices.size(); ++i) {
                for (int j = 0; j < F.cols(); ++j) {
                    submesh.F(i, j) = old_to_new_vert_map[F(selected_face_indices[i], j)];
                }
            }
            return submesh;
        };

        MITSU_Domoe::Polygon_mesh mesh_a_val = create_submesh(faces_a_indices);
        MITSU_Domoe::Polygon_mesh mesh_b_val = create_submesh(faces_b_indices);

        return Output{
            .mesh_a = mesh_a_val,
            .mesh_b = mesh_b_val,
            .message = "Successfully split mesh near x = " + std::to_string(median_x)
        };
    }
};

static_assert(MITSU_Domoe::Cartridge<CutMeshCartridge>);
