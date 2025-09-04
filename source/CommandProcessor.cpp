#include <MITSUDomoe/CommandProcessor.hpp>
#include <iostream>
#include <regex>

namespace MITSU_Domoe
{

    CommandProcessor::CommandProcessor(std::shared_ptr<ResultRepository> repo)
        : result_repo_(std::move(repo)) {}

    CommandProcessor::~CommandProcessor()
    {
        stop();
    }

    void CommandProcessor::start()
    {
        worker_thread_ = std::thread(&CommandProcessor::worker_loop, this);
    }

    void CommandProcessor::stop()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_flag_ = true;
        }
        condition_.notify_one();
        if (worker_thread_.joinable())
        {
            worker_thread_.join();
        }
    }

    void CommandProcessor::worker_loop()
    {
        while (true)
        {
            std::pair<uint64_t, std::function<CommandResult()>> task_pair;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] { return !command_queue_.empty() || stop_flag_; });

                if (stop_flag_ && command_queue_.empty())
                {
                    return;
                }

                task_pair = std::move(command_queue_.front());
                command_queue_.pop();
            }

            auto& [id, task] = task_pair;
            std::cout << "Executing command with ID " << id << "..." << std::endl;
            CommandResult result = task();
            result_repo_->store_result(id, std::move(result));
            std::cout << "Result for command ID " << id << " stored." << std::endl << std::endl;
        }
    }

    std::string CommandProcessor::resolve_refs(const std::string &input_json, const std::string& command_name_of_current_cmd)
    {
        const std::regex ref_regex(R"(\$ref:cmd\[(\d+)\]\.(\w+))");
        auto resolved_json = input_json;

        // This is a simplified search for the key. It assumes one ref per JSON.
        // A more robust solution would parse the JSON and traverse the DOM.
        const std::regex key_finder_regex(R"(\"(\w+)\"\s*:\s*\"\$ref:cmd)");
        std::smatch key_match;
        std::string key_of_ref_field;
        if(std::regex_search(input_json, key_match, key_finder_regex) && key_match.size() > 1) {
            key_of_ref_field = key_match[1].str();
        }

        std::smatch match;
        while (std::regex_search(resolved_json, match, ref_regex))
        {
            const std::string full_match_str = match[0].str();
            const uint64_t cmd_id = std::stoull(match[1].str());
            const std::string member_name = match[2].str();

            // Type Checking Logic
            if (!key_of_ref_field.empty()) {
                const auto& current_cartridge_info = cartridge_manager.at(command_name_of_current_cmd);
                const auto& expected_type = current_cartridge_info.input_schema.arg_names_to_type.at(key_of_ref_field);

                auto referenced_result = result_repo_->get_result(cmd_id);
                if(referenced_result) {
                    auto success_result = std::get_if<SuccessResult>(&(*referenced_result));
                    if(success_result) {
                        const auto& actual_type = success_result->output_schema.at(member_name);
                        if (expected_type != actual_type) {
                            throw std::runtime_error("Type mismatch for field '" + key_of_ref_field + "'. Expected " + expected_type + " but got " + actual_type + ".");
                        }
                    }
                }
            }


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
            task_logic = [this, handler = it->second.handler, input_json, command_name]
            {
                try {
                    const std::string resolved_input_json = this->resolve_refs(input_json, command_name);
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

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            command_queue_.emplace(id, std::move(task_logic));
        }
        condition_.notify_one();

        std::cout << "Command '" << command_name << "' with ID " << id
                  << " added to the queue." << std::endl;
        return id;
    }

    std::vector<std::string> CommandProcessor::get_command_names() const
    {
        std::vector<std::string> names;
        names.reserve(cartridge_manager.size());
        for (const auto &pair : cartridge_manager)
        {
            names.push_back(pair.first);
        }
        return names;
    }

    std::map<std::string, std::string> CommandProcessor::get_input_schema(const std::string& command_name) const
    {
        auto it = cartridge_manager.find(command_name);
        if (it != cartridge_manager.end())
        {
            return it->second.input_schema.arg_names_to_type;
        }
        return {};
    }

} // namespace MITSU_Domoe
