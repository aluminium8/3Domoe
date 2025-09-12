#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include "MITSUDomoe/3D_objects.hpp"
#include <rfl.hpp>
#include <rfl_eigen_serdes.hpp>
#include <igl/doublearea.h>
#include <igl/edge_lengths.h>
#include <vector>
#include <queue>
#include <iostream>

class SubdivisionByCircumradiusCartridge
{
public:
    struct Input
    {
        MITSU_Domoe::Polygon_mesh input_mesh;
        double r_threshold;
    };

    struct Output
    {
        MITSU_Domoe::Polygon_mesh output_mesh;
        std::string message;
    };

    static inline const std::string command_name = "subdivideByCircumradius";
    static inline const std::string description = "Refines a mesh by bisecting any triangle whose circumcircle radius is larger than a threshold.";

    // Helper function to calculate circumradius
    double circumradius(const Eigen::RowVector3d& p1, const Eigen::RowVector3d& p2, const Eigen::RowVector3d& p3) const {
        double a = (p1 - p2).norm();
        double b = (p2 - p3).norm();
        double c = (p3 - p1).norm();

        // Using cross product for area of triangle
        double area = 0.5 * ((p2 - p1).cross(p3 - p1)).norm();

        if (area < 1e-9) { // Avoid division by zero for degenerate triangles
            return std::numeric_limits<double>::max();
        }

        return (a * b * c) / (4.0 * area);
    }

    Output execute(const Input &input) const
    {
        if (input.r_threshold <= 0) {
            return {.output_mesh = input.input_mesh, .message = "Error: r_threshold must be positive."};
        }
        if (input.input_mesh.F.cols() != 3) {
            return {.output_mesh = input.input_mesh, .message = "Error: This cartridge only supports triangle meshes."};
        }

        Eigen::MatrixXd V = input.input_mesh.V;
        Eigen::MatrixXi F = input.input_mesh.F;

        std::vector<Eigen::RowVector3i> faces;
        for (int i = 0; i < F.rows(); ++i) {
            faces.push_back(F.row(i));
        }

        int head = 0;
        while(head < faces.size()){
            Eigen::RowVector3i face = faces[head];

            Eigen::RowVector3d p1 = V.row(face(0));
            Eigen::RowVector3d p2 = V.row(face(1));
            Eigen::RowVector3d p3 = V.row(face(2));

            if (circumradius(p1,p2,p3) > input.r_threshold) {
                Eigen::Vector3d edge_lengths;
                edge_lengths << (p2-p1).norm(), (p3-p2).norm(), (p1-p3).norm();

                int max_idx;
                edge_lengths.maxCoeff(&max_idx);

                int v_idx1 = face(max_idx);
                int v_idx2 = face((max_idx + 1) % 3);

                Eigen::RowVector3d new_vert = (V.row(v_idx1) + V.row(v_idx2)) / 2.0;
                V.conservativeResize(V.rows() + 1, V.cols());
                V.row(V.rows() - 1) = new_vert;
                int new_vert_idx = V.rows() - 1;

                int v_idx_opposite = face((max_idx + 2) % 3);

                // Replace the current face with one of the new ones
                faces[head] = Eigen::RowVector3i(v_idx1, new_vert_idx, v_idx_opposite);
                // Add the other new face to the end of the list
                faces.push_back(Eigen::RowVector3i(v_idx2, v_idx_opposite, new_vert_idx));
            } else {
                head++;
            }
        }

        Eigen::MatrixXi final_F(faces.size(), 3);
        for(size_t i = 0; i < faces.size(); ++i) {
            final_F.row(i) = faces[i];
        }

        MITSU_Domoe::Polygon_mesh result_mesh;
        result_mesh.V = V;
        result_mesh.F = final_F;

        return Output{
            .output_mesh = result_mesh,
            .message = "Successfully subdivided the mesh."
        };
    }
};

static_assert(MITSU_Domoe::Cartridge<SubdivisionByCircumradiusCartridge>);
