#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <MITSUDomoe/CommandProcessor.hpp>
#include <MITSUDomoe/ResultRepository.hpp>

// All cartridges used in the test
#include "ReadStlCartridge.hpp"
#include "GenerateCentroidsCartridge_mock.hpp"
#include "GenerateCentroidsCartridge.hpp" // The new cartridge
#include "BIGprocess_mock_cartridge.hpp"
#include "Need_many_arg_mock_cartridge.hpp"


void print_result(uint64_t id, const std::optional<MITSU_Domoe::CommandResult> &result)
{
    std::cout << "\nClient: Querying result for command ID " << id << "..." << std::endl;
    if (!result)
    {
        std::cout << "  Result not found or not ready." << std::endl;
        return;
    }
    if (const auto *success = std::get_if<MITSU_Domoe::SuccessResult>(&(*result)))
    {
        std::cout << "  Task " << id << " (" << success->command_name << ") Succeeded!" << std::endl;
        std::cout << "  Output json:\n" << success->output_json << std::endl;
    }
    else if (const auto *error = std::get_if<MITSU_Domoe::ErrorResult>(&(*result)))
    {
        std::cout << "  Task " << id << " Failed! Reason: " << error->error_message << std::endl;
    }
}

int main()
{
    auto result_repo = std::make_shared<MITSU_Domoe::ResultRepository>();
    MITSU_Domoe::CommandProcessor processor(result_repo);

    // Register all cartridges
    processor.register_cartridge(ReadStlCartridge{});
    processor.register_cartridge(GenerateCentroidsCartridge_mock{});
    processor.register_cartridge(GenerateCentroidsCartridge{}); // Register the new one
    processor.register_cartridge(BIGprocess_mock_cartridge{});
    processor.register_cartridge(Need_many_arg_mock_cartridge{});

    std::cout << "\n--- Test Case: Chaining commands with $ref ---" << std::endl;

    // 1. Read an STL file. This command will have ID 1.
    const std::string read_stl_input = R"({"filepath": "sample/resource/tetra.stl"})";
    uint64_t read_stl_id = processor.add_to_queue("readStl", read_stl_input);

    // 2. Generate centroids using the output of the previous command.
    // The "$ref" points to the "polygon_mesh" member of the output of command with ID `read_stl_id`.
    const std::string generate_centroids_input =
        R"({"input_polygon_mesh": "$ref:cmd[)" + std::to_string(read_stl_id) + R"(].polygon_mesh"})";
    uint64_t generate_centroids_id = processor.add_to_queue("generateCentroids", generate_centroids_input);

    std::cout << "\nAll commands have been enqueued." << std::endl;

    // Execute the commands
    processor.process_queue();

    // Retrieve and print results
    std::cout << "\n--- Client retrieving results from repository ---" << std::endl;
    print_result(read_stl_id, result_repo->get_result(read_stl_id));
    print_result(generate_centroids_id, result_repo->get_result(generate_centroids_id));

    return 0;
}
