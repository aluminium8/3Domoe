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
        rfl::Field<"filepath", std::string> filepath;
    };

    struct Output
    {
        rfl::Field<"polygon_mesh", MITSU_Domoe::Polygon_mesh> polygon_mesh;
        rfl::Field<"filepath", std::string> filepath;
        rfl::Field<"message", std::string> message;
    };

    static inline const std::string command_name = "readStl";
    static inline const std::string description = "Reads a mesh from an STL file.";

    Output execute(const Input &input) const
    {

        std::ifstream ifs(input.filepath.get());
        Eigen::MatrixXd V;
        Eigen::MatrixXi F;
        Eigen::MatrixXd N; // Normals, not used for now
        if (!igl::readSTL(ifs, V, F, N))
        {
            return Output{
                .polygon_mesh = MITSU_Domoe::Polygon_mesh(),
                .filepath = input.filepath.get(),
                .message = "Failed to read STL file: " + input.filepath.get()
            };
        }

        return Output{
            .polygon_mesh = {{V, F, N}},
            .filepath = input.filepath.get(),
            .message = "Successfully read STL file: " + input.filepath.get()
        };
    }
};

static_assert(MITSU_Domoe::Cartridge<ReadStlCartridge>);
