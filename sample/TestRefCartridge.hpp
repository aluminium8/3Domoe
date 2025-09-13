#pragma once

#include <MITSUDomoe/ICartridge.hpp>
#include <rfl.hpp>
#include <string>

namespace MITSU_Domoe {

struct TestRefCartridge {
    static constexpr char command_name[] = "TestRef";
    static constexpr char description[] = "A cartridge to test $ref resolution from a loaded parameter file.";

    struct Input {
        rfl::Field<"value_from_param", int> value_from_param;
        rfl::Field<"string_from_param", std::string> string_from_param;
    };

    struct Output {
        rfl::Field<"received_value", int> received_value;
        rfl::Field<"received_string", std::string> received_string;
    };

    Output execute(const Input& input) const {
        return Output{
            .received_value = input.value_from_param(),
            .received_string = input.string_from_param()
        };
    }
};

static_assert(Cartridge<TestRefCartridge>);

} // namespace MITSU_Domoe
