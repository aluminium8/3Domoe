#include "MITSUDomoe/ConsoleClient.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
#include <sstream>
#include <fstream>
#include <rfl/json.hpp>

#include "ReadStlCartridge.hpp"
#include "GenerateCentroidsCartridge_mock.hpp"
#include "GenerateCentroidsCartridge.hpp"
#include "BIGprocess_mock_cartridge.hpp"
#include "Need_many_arg_mock_cartridge.hpp"
#include "SubdividePolygonCartridge.hpp"

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
    processor->register_cartridge(SubdividePolygonCartridge{});
}

void ConsoleClient::run()
{
    processor->start();

    std::cout << "MITSUDomoe Console Client. Type 'help' for a list of commands." << std::endl;

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "exit") {
            spdlog::info("Exiting client.");
            break;
        } else if (command == "help") {
            print_help();
        } else if (command == "run_tests") {
            run_tests();
        } else if (command == "load") {
            std::string path;
            ss >> path;
            if (path.empty()) {
                spdlog::error("Usage: load <file_or_directory_path>");
            } else {
                handle_load(path);
            }
        } else if (command == "trace") {
            std::string path;
            ss >> path;
            if (path.empty()) {
                spdlog::error("Usage: trace <file_or_directory_path>");
            } else {
                handle_trace(path);
            }
        } else if (command.empty()) {
            // do nothing
        }
        else {
            spdlog::error("Unknown command: {}", command);
            print_help();
        }
    }
}

void ConsoleClient::print_help() {
    // To be implemented in the next step
    std::cout << "--- MITSUDomoe Help ---\n"
              << "Available commands:\n"
              << "  load <path>      - Loads and displays a JSON log file or all logs in a directory.\n"
              << "  trace <path>     - Re-runs the command from a JSON log file or all logs in a directory.\n"
              << "  run_tests        - Runs the original hardcoded test suite.\n"
              << "  help             - Displays this help message.\n"
              << "  exit             - Exits the application.\n"
              << "-----------------------\n";
}

void ConsoleClient::run_tests() {
    spdlog::info("\n--- Test Case: Chaining commands with $ref (Success) ---");
    const std::string read_stl_input = R"({"filepath": "sample/resource/tetra.stl"})";
    uint64_t read_stl_id = post_command("readStl", read_stl_input);
    const std::string generate_centroids_input =
        R"({"input_polygon_mesh": "$ref:cmd[)" + std::to_string(read_stl_id) + R"(].polygon_mesh"})";
    uint64_t generate_centroids_id = post_command("generateCentroids", generate_centroids_input);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_result(read_stl_id, get_result(read_stl_id));
    print_result(generate_centroids_id, get_result(generate_centroids_id));

    spdlog::info("\n--- Test Case: Subdivide Polygon ---");
    const std::string read_stl_input_3 = R"({"filepath": "sample/resource/tetra.stl"})";
    uint64_t read_stl_id_3 = post_command("readStl", read_stl_input_3);
    const std::string subdivide_input =
        R"({"input_polygon_mesh": "$ref:cmd[)" + std::to_string(read_stl_id_3) + R"(].polygon_mesh", "r": 0.2})";
    uint64_t subdivide_id = post_command("subdividePolygon", subdivide_input);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_result(read_stl_id_3, get_result(read_stl_id_3));
    print_result(subdivide_id, get_result(subdivide_id));


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

    spdlog::info("Main thread: All test cases finished.");
}

void ConsoleClient::handle_load(const std::string& path_str) {
    const std::filesystem::path path(path_str);

    auto process_file = [this](const std::filesystem::path& file_path) {
        if (file_path.extension() != ".json") {
            return;
        }
        spdlog::info("Loading log: {}", file_path.string());
        std::ifstream ifs(file_path);
        if (!ifs) {
            spdlog::error("Failed to open file: {}", file_path.string());
            return;
        }
        const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

        // Store the result from the log in the repository
        this->load_result(content);

        // Pretty-print the JSON
        try {
            auto parsed = rfl::json::read<rfl::Generic>(content);
            if (!parsed) {
                throw std::runtime_error(parsed.error().what());
            }
            const auto pretty_json = rfl::json::write(*parsed, YYJSON_WRITE_PRETTY);
            std::cout << "--- " << file_path.filename().string() << " ---\n"
                      << pretty_json << "\n"
                      << "--------------------" << std::endl;
        } catch (const std::exception& e) {
            spdlog::error("Failed to parse or print JSON from {}: {}", file_path.string(), e.what());
        }
    };

    if (std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                process_file(entry.path());
            }
        }
    } else if (std::filesystem::is_regular_file(path)) {
        process_file(path);
    } else {
        spdlog::error("Path is not a valid file or directory: {}", path_str);
    }
}

void ConsoleClient::handle_trace(const std::string& path_str) {
    const std::filesystem::path path(path_str);

    auto process_file = [this](const std::filesystem::path& file_path) {
        if (file_path.extension() != ".json") {
            return;
        }
        spdlog::info("Tracing log: {}", file_path.string());
        std::ifstream ifs(file_path);
        if (!ifs) {
            spdlog::error("Failed to open file: {}", file_path.string());
            return;
        }
        const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

        try {
            // Define a struct to parse the relevant fields from the log file
            struct Log {
                rfl::Field<"command", std::string> command;
                rfl::Field<"request", rfl::Generic> request;
            };

            auto parsed = rfl::json::read<Log>(content);
            if (!parsed) {
                throw std::runtime_error(parsed.error().what());
            }

            const std::string& command_name = parsed->command();
            const std::string request_json = rfl::json::write(parsed->request());

            spdlog::info("Re-posting command '{}' with input: {}", command_name, request_json);
            this->post_command(command_name, request_json);

        } catch (const std::exception& e) {
            spdlog::error("Failed to parse or trace JSON from {}: {}", file_path.string(), e.what());
        }
    };

    if (std::filesystem::is_directory(path)) {
        // When tracing a directory, it's important to process files in order.
        // The log files are named with a zero-padded ID, so lexicographical sort is correct.
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
        for (const auto& file_path : files) {
            process_file(file_path);
        }
    } else if (std::filesystem::is_regular_file(path)) {
        process_file(path);
    } else {
        spdlog::error("Path is not a valid file or directory: {}", path_str);
    }
}


}
