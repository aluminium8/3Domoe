#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include "MITSUDomoe/3D_objects.hpp"
#include <rfl.hpp>
#include <rfl_eigen_serdes.hpp>
#include <Eigen/Core>
#include <string>
#include <vector>

class GenerateCentroidsCartridge
{
public:
    struct Input
    {
        rfl::Field<"input_polygon_mesh", MITSU_Domoe::Polygon_mesh> input_polygon_mesh;
    };

    struct Output
    {
        rfl::Field<"centroids", Eigen::MatrixXd> centroids;
        rfl::Field<"message", std::string> message;
    };

    static inline const std::string command_name = "generateCentroids";
    static inline const std::string description = "Calculates the centroid of each face in a polygon mesh.";

    Output execute(const Input &input) const
    {
        const auto& V = input.input_polygon_mesh.get().V;
        const auto& F = input.input_polygon_mesh.get().F;

        if (F.rows() == 0) {
            return Output{
                .centroids = Eigen::MatrixXd(),
                .message = "Input mesh has no faces."
            };
        }

        Eigen::MatrixXd centroids(F.rows(), V.cols());
        for (int i = 0; i < F.rows(); ++i)
        {
            Eigen::RowVector3d face_centroid = Eigen::RowVector3d::Zero();
            for (int j = 0; j < F.cols(); ++j)
            {
                face_centroid += V.row(F(i, j));
            }
            centroids.row(i) = face_centroid / F.cols();
        }

        const auto num_centroids = centroids.rows();

        return Output{
            .centroids = std::move(centroids),
            .message = "Successfully generated " + std::to_string(num_centroids) + " centroids."
        };
    }
};

static_assert(MITSU_Domoe::Cartridge<GenerateCentroidsCartridge>);
