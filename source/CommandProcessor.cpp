#include <MITSUDomoe/CommandProcessor.hpp>
#include <iostream>
#include <regex>

namespace MITSU_Domoe
{

    CommandProcessor::CommandProcessor(std::shared_ptr<ResultRepository> repo)
        : result_repo_(std::move(repo)) {}

    std::string CommandProcessor::resolve_refs(const std::string &input_json)
    {
        const std::regex ref_regex(R"(\$ref:cmd\[(\d+)\]\.(\w+))");
        auto resolved_json = input_json;
        std::smatch match;

        while (std::regex_search(resolved_json, match, ref_regex))
        {
            const std::string full_match_str = match[0].str();
            const uint64_t cmd_id = std::stoull(match[1].str());
            const std::string member_name = match[2].str();

            auto result = result_repo_->get_result(cmd_id);
            if (!result) {
                throw std::runtime_error("Referenced command with ID " + std::to_string(cmd_id) + " not found.");
            }

            auto success_result = std::get_if<SuccessResult>(&(*result));
            if (!success_result) {
                throw std::runtime_error("Referenced command with ID " + std::to_string(cmd_id) + " failed.");
            }

            yyjson_doc *doc = yyjson_read(success_result->output_json.c_str(), success_result->output_json.length(), 0);
            if (!doc) {
                throw std::runtime_error("Failed to parse output JSON for command ID " + std::to_string(cmd_id));
            }

            yyjson_val *root = yyjson_doc_get_root(doc);
            if (!yyjson_is_obj(root)) {
                yyjson_doc_free(doc);
                throw std::runtime_error("Output of command ID " + std::to_string(cmd_id) + " is not a JSON object.");
            }

            yyjson_val *member_val = yyjson_obj_get(root, member_name.c_str());
            if (!member_val) {
                yyjson_doc_free(doc);
                throw std::runtime_error("Member '" + member_name + "' not found in output of command ID " + std::to_string(cmd_id));
            }

            const char *member_json_c_str = yyjson_val_write(member_val, 0, NULL);
            std::string member_json(member_json_c_str);
            free((void*)member_json_c_str);
            yyjson_doc_free(doc);

            std::string quoted_ref = "\"" + full_match_str + "\"";
            size_t pos = resolved_json.find(quoted_ref);
            if (pos != std::string::npos)
            {
                resolved_json.replace(pos, quoted_ref.length(), member_json);
            }
        }
        return resolved_json;
    }

    uint64_t CommandProcessor::add_to_queue(const std::string &command_name, const std::string &input_json)
    {
        const uint64_t id = next_command_id_++;
        std::function<CommandResult()> task_logic;

        if (auto it = cartridge_manager.find(command_name); it != cartridge_manager.end())
        {
            // The handler now also takes the original input_json.
            // Resolution will happen at execution time in process_queue.
            task_logic = [this, handler = it->second.handler, input_json]
            {
                // The actual handler is called inside a new lambda that first resolves refs.
                try {
                    const std::string resolved_input_json = this->resolve_refs(input_json);
                    return handler(resolved_input_json);
                } catch (const std::exception& e) {
                    return CommandResult(ErrorResult{"Error during reference resolution: " + std::string(e.what())});
                }
            };
        }
        else
        {
            task_logic = [command_name]
            {
                return ErrorResult{"Error: Command '" + command_name + "' not found."};
            };
        }

        command_queue_.emplace(id, std::move(task_logic));
        std::cout << "Command '" << command_name << "' with ID " << id
                  << " added to the queue." << std::endl;
        return id;
    }

    void CommandProcessor::process_queue()
    {
        std::cout << "\n--- Processing command queue (" << command_queue_.size()
                  << " commands) ---" << std::endl;
        while (!command_queue_.empty())
        {
            auto [id, task] = std::move(command_queue_.front());
            command_queue_.pop();
            std::cout << "Executing command with ID " << id << "..." << std::endl;
            // The reference resolution is now part of the task itself.
            CommandResult result = task();
            result_repo_->store_result(id, std::move(result));
            std::cout << "Result for command ID " << id << " stored." << std::endl
                      << std::endl;
        }
        std::cout << "--- Command queue processed ---" << std::endl;
    }

} // namespace MITSU_Domoe
