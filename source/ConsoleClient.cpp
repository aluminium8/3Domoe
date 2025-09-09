#include "MITSUDomoe/ConsoleClient.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>

#include "ReadStlCartridge.hpp"
#include "GenerateCentroidsCartridge_mock.hpp"
#include "GenerateCentroidsCartridge.hpp"
#include "BIGprocess_mock_cartridge.hpp"
#include "Need_many_arg_mock_cartridge.hpp"

namespace
{
void print_result(uint64_t id, const std::optional<MITSU_Domoe::CommandResult> &result)
{
    spdlog::info("Client: Querying result for command ID {}...", id);
    if (!result)
    {
        spdlog::warn("  Result not found or not ready.");
        return;
    }
    if (const auto *success = std::get_if<MITSU_Domoe::SuccessResult>(&(*result)))
    {
        spdlog::info("  Task {} ({}) Succeeded!", id, success->command_name);
        spdlog::info("  Output json:\n{}", success->output_json);
    }
    else if (const auto *error = std::get_if<MITSU_Domoe::ErrorResult>(&(*result)))
    {
        spdlog::error("  Task {} Failed! Reason: {}", id, error->error_message);
    }
}
}

namespace MITSU_Domoe
{

ConsoleClient::ConsoleClient(const std::filesystem::path& log_path) : BaseClient(log_path)
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

    spdlog::info("\n--- Test Case: Chaining commands with $ref (Success) ---");
    const std::string read_stl_input = R"({"filepath": "sample/resource/tetra.stl"})";
    uint64_t read_stl_id = post_command("readStl", read_stl_input);
    const std::string generate_centroids_input =
        R"({"input_polygon_mesh": "$ref:cmd[)" + std::to_string(read_stl_id) + R"(].polygon_mesh"})";
    uint64_t generate_centroids_id = post_command("generateCentroids", generate_centroids_input);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_result(read_stl_id, get_result(read_stl_id));
    print_result(generate_centroids_id, get_result(generate_centroids_id));

    spdlog::info("\n--- Test Case: Chaining commands with $ref (Type Mismatch) ---");
    const std::string read_stl_input_2 = R"({"filepath": "sample/resource/tetra.stl"})";
    uint64_t read_stl_id_2 = post_command("readStl", read_stl_input_2);

    const std::string bad_ref_input =
        R"({"input_mesh_id": "$ref:cmd[)" + std::to_string(read_stl_id_2) + R"(].polygon_mesh"})";
    uint64_t bad_ref_id = post_command("GenerateCentroids_mock", bad_ref_input);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_result(read_stl_id_2, get_result(read_stl_id_2));
    print_result(bad_ref_id, get_result(bad_ref_id));

    spdlog::info("\n--- Test Case: Asynchronous command execution ---");
    const std::string big_process_input = R"({"time_to_process": 5})";
    uint64_t big_process_id = post_command("BIGprocess_mock", big_process_input);
    spdlog::info("Main thread: BIGprocess_mock_cartridge added to queue. Main thread is NOT blocked.");

    while(true) {
        auto result = get_result(big_process_id);
        if (result) {
            print_result(big_process_id, result);
            break;
        }
        spdlog::info("Main thread: Waiting for BIGprocess_mock_cartridge to finish...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    spdlog::info("Main thread: All test cases finished. Stopping processor.");
}

}
