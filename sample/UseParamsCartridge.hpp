#pragma once

#include <MITSUDomoe/ICartridge.hpp>
#include <rfl.hpp>
#include <string>
#include <iostream>

namespace MITSU_Domoe
{

    struct UseParamsCartridge
    {
        static constexpr char const *command_name = "useParams";
        static constexpr char const *description = "A cartridge to demonstrate using loaded parameters.";

        struct Input
        {
            rfl::Field<"message", std::string> message;
            rfl::Field<"value", int> value;
        };

        struct Output
        {
            rfl::Field<"status", std::string> status;
        };

        Output execute(const Input &input) const
        {
            std::cout << "UseParamsCartridge executed!" << std::endl;
            std::cout << "  Message: " << input.message() << std::endl;
            std::cout << "  Value: " << input.value() << std::endl;
            return Output{"Parameters received successfully."};
        }
    };

} // namespace MITSU_Domoe
