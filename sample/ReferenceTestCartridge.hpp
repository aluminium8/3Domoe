#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include <rfl/json.hpp>
#include <string>

class ReferenceTestCartridge {
public:
    struct Input {
        rfl::Field<"some_data", rfl::Generic> some_data;
    };

    struct Output {
        rfl::Field<"message", std::string> message;
        rfl::Field<"received_data_json", std::string> received_data_json;
    };

    static inline const std::string command_name = "reference_test";
    static inline const std::string description = "Tests receiving generic data, likely from a $ref.";

    Output execute(const Input& input) const {
        std::string received_json = rfl::json::write(input.some_data.get());

        if (received_json.empty() || received_json == "null") {
            throw std::runtime_error("Received data was empty or null.");
        }

        return Output{
            .message = "Successfully received and validated the referenced data.",
            .received_data_json = std::move(received_json)
        };
    }
};

static_assert(MITSU_Domoe::Cartridge<ReferenceTestCartridge>);
