#include <MITSUDomoe/CommandProcessor.hpp>
#include <iostream>

namespace MITSU_Domoe {

// (中身は前回と同じ)
CommandProcessor::CommandProcessor(std::shared_ptr<ResultRepository> repo)
    : result_repo_(std::move(repo)) {}

uint64_t CommandProcessor::add_to_queue(const std::string& command_name, const std::string& input_toml) {
    const uint64_t id = next_command_id_++;
    std::function<CommandResult()> task_logic;
    if (auto it = cartridge_manager.find(command_name); it != cartridge_manager.end()) {
        task_logic = [handler = it->second.handler, input_toml] {
            return handler(input_toml);
        };
    } else {
        task_logic = [command_name] {
            return ErrorResult{"Error: Command '" + command_name + "' not found."};
        };
    }
    command_queue_.emplace(id, std::move(task_logic));
    std::cout << "Command '" << command_name << "' with ID " << id
              << " added to the queue." << std::endl;
    return id;
}

void CommandProcessor::process_queue() {
    std::cout << "\n--- Processing command queue (" << command_queue_.size()
              << " commands) ---" << std::endl;
    while (!command_queue_.empty()) {
        auto [id, task] = std::move(command_queue_.front());
        command_queue_.pop();
        std::cout << "Executing command with ID " << id << "..." << std::endl;
        CommandResult result = task();
        result_repo_->store_result(id, std::move(result));
        std::cout << "Result for command ID " << id << " stored." << std::endl<< std::endl;
    }
    std::cout << "--- Command queue processed ---" << std::endl;
}

} // namespace MITSU_Domoe
