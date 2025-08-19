#include <iostream>
#include <memory>
#include <optional>
#include <vector>

// インクルードパスを新しい構成に合わせる
#include <MITSUDomoe/CommandProcessor.hpp>
#include <MITSUDomoe/ResultRepository.hpp>
#include "GenerateCentroidsCartridge_mock.hpp"
#include "BIGprocess_mock_cartridge.hpp"

// (関数の中身は前回と同じ)
void print_result(uint64_t id, const std::optional<MITSU_Domoe::CommandResult>& result) {
    std::cout << "\nClient: Querying result for command ID " << id << "..." << std::endl;
    if (!result) {
        std::cout << "  Result not found or not ready." << std::endl;
        return;
    }
    if (const auto* success = std::get_if<MITSU_Domoe::SuccessResult>(&(*result))) {
        std::cout << "  Task " << id << " Succeeded!" << std::endl;
        std::cout << "  Raw Output TOML:\n" << success->output_toml << std::endl;
        auto output = rfl::toml::read<GenerateCentroidsCartridge_mock::Output>(
            success->output_toml);
        if (output) {
            std::cout << "  Deserialized Message: " << output->message << std::endl;
        } else {
            std::cout << "  Failed to deserialize success result." << std::endl;
        }
    } else if (const auto* error = std::get_if<MITSU_Domoe::ErrorResult>(&(*result))) {
        std::cout << "  Task " << id << " Failed! Reason: " << error->error_message
                  << std::endl;
    }
}

int main() {
    // (中身は前回と同じ)
    auto result_repo = std::make_shared<MITSU_Domoe::ResultRepository>();
    MITSU_Domoe::CommandProcessor processor(result_repo);
    processor.register_cartridge("generateCentroids", GenerateCentroidsCartridge_mock{});
    processor.register_cartridge("BIGprocess_mock_cartridge",BIGprocess_mock_cartridge{});

    std::cout << "\n--- Cartridge Schemas ---" << std::endl;
    auto schemas = processor.get_cartridge_schemas();
    for (const auto& [name, schema] : schemas) {
        std::cout << "Cartridge: " << name << "\nSchema: " << schema << std::endl;
    }

    const std::string command_name = "generateCentroids";
    const std::string input_toml1 = R"(input_mesh_id = 101)";
    const std::string input_toml2 = R"(input_mesh_id = 202)";
    const std::string command_name2 = "BIGprocess_mock_cartridge";


    const std::string invalid_command = "nonExistentCommand";
    const std::string invalid_toml = R"(invalid_field = 999)";

    std::cout << "\n--- Client adding commands to the queue ---" << std::endl;
    uint64_t id1 = processor.add_to_queue(command_name, input_toml1);
    uint64_t id2 = processor.add_to_queue(command_name2, input_toml2);
    uint64_t id3 = processor.add_to_queue(invalid_command, input_toml1);
    uint64_t id4 = processor.add_to_queue(command_name2, invalid_toml);
    uint64_t id5 = processor.add_to_queue(command_name2, input_toml1);
    std::cout << "\nAll commands have been enqueued. No execution has happened yet." << std::endl;

    processor.process_queue();

    std::cout << "\n--- Client retrieving results from repository ---" << std::endl;
    print_result(id1, result_repo->get_result(id1));
    print_result(id2, result_repo->get_result(id2));
    print_result(id3, result_repo->get_result(id3));
    print_result(id4, result_repo->get_result(id4));
    print_result(id5, result_repo->get_result(id4));
    return 0;
}
