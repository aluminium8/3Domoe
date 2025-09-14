#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include <fstream>
#include <rfl.hpp>
#include <string>

class LoadJsonCartridge
{
public:
    struct Input
    {
        rfl::Field<"filepath", std::string> filepath;
    };

    struct Output
    {
        rfl::Field<"data", rfl::Generic> data;
        rfl::Field<"message", std::string> message;
    };

    static inline const std::string command_name = "loadJson";
    static inline const std::string description = "Loads a JSON file and registers its content as a result.";

    Output execute(const Input &input) const
    {
        const std::string& path = input.filepath.get();
        std::ifstream ifs(path);
        if (!ifs)
        {
            return Output{rfl::Generic(), "Failed to open file: " + path};
        }

        const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

        auto parsed = rfl::json::read<rfl::Generic>(content);
        if (!parsed)
        {
            return Output{rfl::Generic(), "Failed to parse JSON from file: " + path};
        }

        return Output{std::move(*parsed), "Successfully loaded JSON from " + path};
    }
};

static_assert(MITSU_Domoe::Cartridge<LoadJsonCartridge>);
