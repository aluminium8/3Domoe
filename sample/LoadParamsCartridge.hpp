#pragma once

#include <MITSUDomoe/ICartridge.hpp>
#include <rfl.hpp>
#include <filesystem>
#include <functional>
#include <string>

namespace MITSU_Domoe {

struct LoadParamsCartridge {
    struct Input {
        std::string file_path;
    };

    struct Output {
        bool success;
        std::string message;
        uint64_t loaded_id;
    };

    static constexpr char const* command_name = "load_params";
    static constexpr char const* description = "Loads a parameter file into the result repository.";

    std::function<uint64_t(const std::filesystem::path&)> load_func;

    Output execute(const Input& input) const {
        try {
            uint64_t id = load_func(input.file_path);
            if (id > 0) {
                return {true, "Parameters loaded successfully.", id};
            } else {
                return {false, "Failed to load parameters.", 0};
            }
        } catch (const std::exception& e) {
            return {false, "Exception while loading parameters: " + std::string(e.what()), 0};
        }
    }
};

}
