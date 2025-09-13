#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <rfl/json.hpp>

class LoadJsonCartridge {
public:
    struct Input {
        rfl::Field<"filepath", std::string> filepath;
    };

    // The output is a generic JSON object, which can hold any structure.
    struct Output {
        rfl::Field<"data", rfl::Generic> data;
        rfl::Field<"filepath", std::string> filepath;
    };

    static inline const std::string command_name = "load_json_file";
    static inline const std::string description = "Loads parameters from a JSON file.";

    Output execute(const Input& input) const {
        const std::string path = input.filepath.get();
        std::ifstream ifs(path);
        if (!ifs) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::stringstream buffer;
        buffer << ifs.rdbuf();
        const std::string content = buffer.str();

        auto parsed_data = rfl::json::read<rfl::Generic>(content);
        if (!parsed_data) {
            throw std::runtime_error("Failed to parse JSON from file: " + path + ". Error: " + parsed_data.error().what());
        }

        return Output{
            .data = std::move(*parsed_data),
            .filepath = path
        };
    }
};

// Verify that the cartridge conforms to the concept at compile time.
static_assert(MITSU_Domoe::Cartridge<LoadJsonCartridge>);
