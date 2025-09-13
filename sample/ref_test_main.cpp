#include <MITSUDomoe/CommandProcessor.hpp>
#include <MITSUDomoe/ResultRepository.hpp>
#include <MITSUDomoe/Logger.hpp>
#include "TestRefCartridge.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

void run_live_test(MITSU_Domoe::CommandProcessor& processor) {
    spdlog::info("---- Running Live $ref Test Sequence ----");

    // 1. Load the parameter file
    std::string load_params_input = R"({"file_path": "params.json"})";
    uint64_t load_cmd_id = processor.add_to_queue("load_params", load_params_input);
    spdlog::info("Queued load_params command with ID: {}", load_cmd_id);

    // Give it a moment to process before the next command
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 2. Run the test cartridge that uses refs to the loaded file
    std::string test_ref_input = R"({"value_from_param": $ref:latest.my_value, "string_from_param": "$ref:latest.another_value"})";
    uint64_t test_cmd_id = processor.add_to_queue("TestRef", test_ref_input);
    spdlog::info("Queued TestRef command with ID: {}", test_cmd_id);

    spdlog::info("---- Live test sequence queued. ----");
}

void run_history_test(const std::filesystem::path& log_path) {
    spdlog::info("---- Running History/Persistence Test Sequence ----");

    auto result_repo = std::make_shared<MITSU_Domoe::ResultRepository>();
    auto command_processor = std::make_shared<MITSU_Domoe::CommandProcessor>(result_repo, log_path);
    command_processor->start();

    // 1. Manually load the result of the 'load_params' command from the log
    // In a real app, this would be part of a loop that loads all history.
    std::filesystem::path history_file_path = log_path / "0001_load_params.json";
    spdlog::info("Attempting to load history from: {}", history_file_path.string());

    std::ifstream history_file(history_file_path);
    if (!history_file.is_open()) {
        spdlog::error("FATAL: Could not open history file to verify persistence. {}", history_file_path.string());
        return;
    }
    std::string content((std::istreambuf_iterator<char>(history_file)), std::istreambuf_iterator<char>());

    command_processor->load_result_from_log(content);
    spdlog::info("Loaded result from log for command ID 1.");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 2. Register the cartridge needed for the test
    command_processor->register_cartridge(MITSU_Domoe::TestRefCartridge{});

    // 3. Run a new command referencing the *loaded* result by its specific ID
    std::string test_ref_input = R"({"value_from_param": $ref:cmd[1].my_value, "string_from_param": "$ref:cmd[1].another_value"})";
    uint64_t test_cmd_id = command_processor->add_to_queue("TestRef", test_ref_input);
    spdlog::info("Queued TestRef command with ID {} to test persistence.", test_cmd_id);

    spdlog::info("---- History test sequence queued. ----");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    command_processor->stop();
}


int main(int argc, char* argv[])
{
    auto log_path = MITSU_Domoe::initialize_logger();

    // Setup processor for the first run
    auto result_repo = std::make_shared<MITSU_Domoe::ResultRepository>();
    auto command_processor = std::make_shared<MITSU_Domoe::CommandProcessor>(result_repo, log_path);
    command_processor->register_cartridge(MITSU_Domoe::TestRefCartridge{});
    command_processor->start();

    // Run the first part of the test (live loading and referencing)
    run_live_test(*command_processor);

    // Wait for the live test to finish processing
    std::this_thread::sleep_for(std::chrono::seconds(3));
    command_processor->stop();

    // Run the second part of the test (persistence)
    // This simulates an application restart
    run_history_test(log_path);

    spdlog::info("---- Full test sequence finished. Check logs for results. ----");

    return 0;
}
