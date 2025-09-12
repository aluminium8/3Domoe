#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include "MITSUDomoe/3D_objects.hpp"
#include <rfl.hpp>
#include <rfl_eigen_serdes.hpp>
#include <Eigen/Core>
#include <string>
#include <vector>
#include <igl/barycenter.h>
#include <igl/edge_lengths.h>
#include <iostream>
#include <list>

class SubdividePolygonCartridge
{
public:
    struct Input
    {
        rfl::Field<"input_polygon_mesh", MITSU_Domoe::Polygon_mesh> input_polygon_mesh;
        rfl::Field<"r", double> r;
    };

    struct Output
    {
        rfl::Field<"output_polygon_mesh", MITSU_Domoe::Polygon_mesh> output_polygon_mesh;
        rfl::Field<"num_faces_before", int> polygon_num_origin;
        rfl::Field<"num_faces_after", int> polygon_num_subdivided ;
        rfl::Field<"message", std::string> message;
    };

    static inline const std::string command_name = "subdividePolygon";
    static inline const std::string description = "Subdivides polygons until the max distance from centroid to vertex is within a threshold 'r'.";

    Output execute(const Input &input) const
    {
        Eigen::MatrixXd V = input.input_polygon_mesh.get().V;
        Eigen::MatrixXi F = input.input_polygon_mesh.get().F;
        const double r = input.r.get();
        const double r_squared = r * r;

        const int num_faces_before = F.rows();

        std::vector<Eigen::RowVector3d> V_vec;
        for (long i = 0; i < V.rows(); ++i)
        {
            V_vec.push_back(V.row(i));
        }

        std::list<Eigen::RowVector3i> F_worklist;
        for (long i = 0; i < F.rows(); ++i)
        {
            F_worklist.push_back(F.row(i));
        }

        std::vector<Eigen::RowVector3i> F_final_vec;

        while (!F_worklist.empty())
        {
            Eigen::RowVector3i face_indices = F_worklist.front();
            F_worklist.pop_front();

            Eigen::RowVector3d v0 = V_vec[face_indices(0)];
            Eigen::RowVector3d v1 = V_vec[face_indices(1)];
            Eigen::RowVector3d v2 = V_vec[face_indices(2)];

            Eigen::RowVector3d centroid = (v0 + v1 + v2) / 3.0;

            double max_dist_sq = 0.0;
            max_dist_sq = std::max(max_dist_sq, (v0 - centroid).squaredNorm());
            max_dist_sq = std::max(max_dist_sq, (v1 - centroid).squaredNorm());
            max_dist_sq = std::max(max_dist_sq, (v2 - centroid).squaredNorm());

            if (max_dist_sq <= r_squared)
            {
                F_final_vec.push_back(face_indices);
            }
            else
            {
                double d01_sq = (v0 - v1).squaredNorm();
                double d12_sq = (v1 - v2).squaredNorm();
                double d20_sq = (v2 - v0).squaredNorm();

                int v_idx0 = face_indices(0);
                int v_idx1 = face_indices(1);
                int v_idx2 = face_indices(2);

                int p1_idx, p2_idx, opposite_v_idx;
                Eigen::RowVector3d new_v_coords;

                if (d01_sq >= d12_sq && d01_sq >= d20_sq)
                {
                    p1_idx = v_idx0;
                    p2_idx = v_idx1;
                    opposite_v_idx = v_idx2;
                    new_v_coords = (v0 + v1) / 2.0;
                }
                else if (d12_sq >= d01_sq && d12_sq >= d20_sq)
                {
                    p1_idx = v_idx1;
                    p2_idx = v_idx2;
                    opposite_v_idx = v_idx0;
                    new_v_coords = (v1 + v2) / 2.0;
                }
                else
                {
                    p1_idx = v_idx2;
                    p2_idx = v_idx0;
                    opposite_v_idx = v_idx1;
                    new_v_coords = (v2 + v0) / 2.0;
                }

                int new_v_idx = V_vec.size();
                V_vec.push_back(new_v_coords);

                F_worklist.push_back(Eigen::RowVector3i(p1_idx, new_v_idx, opposite_v_idx));
                F_worklist.push_back(Eigen::RowVector3i(p2_idx, opposite_v_idx, new_v_idx));
            }
        }

        Eigen::MatrixXd V_out(V_vec.size(), 3);
        for (size_t i = 0; i < V_vec.size(); ++i)
        {
            V_out.row(i) = V_vec[i];
        }

        Eigen::MatrixXi F_out(F_final_vec.size(), 3);
        for (size_t i = 0; i < F_final_vec.size(); ++i)
        {
            F_out.row(i) = F_final_vec[i];
        }
        const int num_faces_after = F_out.rows();
        return Output{
            .output_polygon_mesh = MITSU_Domoe::Polygon_mesh{V_out, F_out, {}},
            .polygon_num_origin = num_faces_before,
            .polygon_num_subdivided = num_faces_after,
            .message = "Successfully subdivided mesh. Original faces: " + std::to_string(F.rows()) + ", new faces: " + std::to_string(F_out.rows())};
    }
};

static_assert(MITSU_Domoe::Cartridge<SubdividePolygonCartridge>);
