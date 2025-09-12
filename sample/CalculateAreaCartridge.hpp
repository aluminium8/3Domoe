#pragma once

#include <MITSUDomoe/ICartridge.hpp>
#include <rfl.hpp>

namespace MITSU_Domoe {

struct CalculateAreaCartridge {
    struct Input {
        float length;
        float width;
    };

    struct Output {
        float area;
    };

    static constexpr char const* command_name = "calculate_area";
    static constexpr char const* description = "Calculates the area of a rectangle.";

    Output execute(const Input& input) const {
        return {input.length * input.width};
    }
};

}
