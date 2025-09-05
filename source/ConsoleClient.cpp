#include "MITSUDomoe/ConsoleClient.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#include "ReadStlCartridge.hpp"
#include "GenerateCentroidsCartridge_mock.hpp"
#include "GenerateCentroidsCartridge.hpp"
#include "BIGprocess_mock_cartridge.hpp"
#include "Need_many_arg_mock_cartridge.hpp"

namespace
{
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
}

namespace MITSU_Domoe
{

ConsoleClient::ConsoleClient()
{
    processor->register_cartridge(ReadStlCartridge{});
    processor->register_cartridge(GenerateCentroidsCartridge_mock{});
    processor->register_cartridge(GenerateCentroidsCartridge{});
    processor->register_cartridge(BIGprocess_mock_cartridge{});
    processor->register_cartridge(Need_many_arg_mock_cartridge{});
}

void ConsoleClient::run()
{
    processor->start();

    std::cout << "\n--- Test Case: Chaining commands with $ref (Success) ---" << std::endl;
    const std::string read_stl_input = R"({"filepath": "sample/resource/tetra.stl"})";
    uint64_t read_stl_id = post_command("readStl", read_stl_input);
    const std::string generate_centroids_input =
        R"({"input_polygon_mesh": "$ref:cmd[)" + std::to_string(read_stl_id) + R"(].polygon_mesh"})";
    uint64_t generate_centroids_id = post_command("generateCentroids", generate_centroids_input);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_result(read_stl_id, get_result(read_stl_id));
    print_result(generate_centroids_id, get_result(generate_centroids_id));

    std::cout << "\n--- Test Case: Chaining commands with $ref (Type Mismatch) ---" << std::endl;
    const std::string read_stl_input_2 = R"({"filepath": "sample/resource/tetra.stl"})";
    uint64_t read_stl_id_2 = post_command("readStl", read_stl_input_2);

    const std::string bad_ref_input =
        R"({"input_mesh_id": "$ref:cmd[)" + std::to_string(read_stl_id_2) + R"(].polygon_mesh"})";
    uint64_t bad_ref_id = post_command("GenerateCentroids_mock", bad_ref_input);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_result(read_stl_id_2, get_result(read_stl_id_2));
    print_result(bad_ref_id, get_result(bad_ref_id));

    std::cout << "\n--- Test Case: Asynchronous command execution ---" << std::endl;
    const std::string big_process_input = R"({"time_to_process": 5})";
    uint64_t big_process_id = post_command("BIGprocess_mock", big_process_input);
    std::cout << "Main thread: BIGprocess_mock_cartridge added to queue. Main thread is NOT blocked." << std::endl;

    while(true) {
        auto result = get_result(big_process_id);
        if (result) {
            print_result(big_process_id, result);
            break;
        }
        std::cout << "Main thread: Waiting for BIGprocess_mock_cartridge to finish..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Main thread: All test cases finished. Stopping processor." << std::endl;
}

}
