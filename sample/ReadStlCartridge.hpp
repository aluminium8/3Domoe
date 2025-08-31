#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include <fstream>
#include <rfl.hpp>
#include <rfl_eigen_serdes.hpp> // Use the serialization library for Eigen
#include <igl/readSTL.h>
#include <Eigen/Core>
#include <vector>
#include <string>

#include "MITSUDomoe/3D_objects.hpp"

class ReadStlCartridge
{
public:
    struct Input
    {
        std::string filepath;
    };

    struct Output
    {
        MITSU_Domoe::Polygon_mesh polygon_mesh;
        std::string message;
    };

    static inline const std::string command_name = "readStl";
    static inline const std::string description = "Reads a mesh from an STL file.";

    Output execute(const Input &input) const
    {

        std::ifstream ifs(input.filepath);
        Eigen::MatrixXd V;
        Eigen::MatrixXi F;
        Eigen::MatrixXd N; // Normals, not used for now
        if (!igl::readSTL(ifs, V, F, N))
        {
            return Output{
                .message = "Failed to read STL file: " + input.filepath};
        }

        Output output;
        output.polygon_mesh.V = std::move(V);
        output.polygon_mesh.F = std::move(F);
        output.polygon_mesh.N = std::move(N);
        output.message = "Successfully read STL file: " + input.filepath;
        return output;
    }
};

static_assert(MITSU_Domoe::Cartridge<ReadStlCartridge>);
