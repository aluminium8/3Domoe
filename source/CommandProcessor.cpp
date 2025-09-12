#include <MITSUDomoe/CommandProcessor.hpp>
#include <MITSUDomoe/Logger.hpp>
#include <iostream>
#include <regex>
#include <spdlog/spdlog.h>
#include <fstream>
#include <rfl/json.hpp>
#include <iomanip>

namespace MITSU_Domoe
{
    namespace
    {
        void log_unresolved_command(uint64_t id, const std::string &command_name, const std::string &unresolved_json, const std::filesystem::path &command_history_path)
        {
            std::stringstream ss;
            ss << "{";
            ss << "\"command\":\"" << command_name << "\",";
            ss << "\"request\":" << unresolved_json;
            ss << "}";

            std::stringstream filename_ss;
            filename_ss << std::setw(LOG_ID_PADDING) << std::setfill('0') << id
                        << "_" << command_name << ".json";

            try
            {
                std::ofstream log_file(command_history_path / filename_ss.str());
                if (log_file)
                {
                    log_file << ss.str();
                    spdlog::info("Successfully logged unresolved command {} to history.", id);
                }
                else
                {
                    spdlog::error("Failed to open command history file for writing: {}", (command_history_path / filename_ss.str()).string());
                }
            }
            catch (const std::exception &e)
            {
                spdlog::error("Exception while writing to command history: {}", e.what());
            }
        }
    }

    CommandProcessor::CommandProcessor(std::shared_ptr<ResultRepository> repo, const std::filesystem::path &log_path)
        : result_repo_(std::move(repo)), log_path_(log_path)
    {
        command_history_path_ = log_path_ / "command_history";
        try
        {
            if (!std::filesystem::exists(command_history_path_))
            {
                std::filesystem::create_directories(command_history_path_);
                spdlog::info("Created command_history directory at: {}", command_history_path_.string());
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            spdlog::error("Failed to create command_history directory: {}", e.what());
        }
    }

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
            CommandTask current_task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this]
                                { return !command_queue_.empty() || stop_flag_; });

                if (stop_flag_ && command_queue_.empty())
                {
                    return;
                }

                current_task = std::move(command_queue_.front());
                command_queue_.pop();
            }

            spdlog::info("Executing command '{}' with ID {}...", current_task.command_name, current_task.id);
            CommandResult result = current_task.task();

            std::stringstream ss;
            ss << "{";
            ss << "\"id\":" << current_task.id << ",";
            ss << "\"command\":\"" << current_task.command_name << "\",";

            if (const auto *success = std::get_if<SuccessResult>(&result))
            {
                ss << "\"unresolved_request\":" << success->unresolved_input_json << ",";
                ss << "\"request\":" << success->resolved_input_json << ",";
                ss << "\"status\":\"success\",";
                ss << "\"response\":" << success->output_json << ",";
                ss << "\"schema\":" << rfl::json::write(success->output_schema);
            }
            else if (const auto *error = std::get_if<ErrorResult>(&result))
            {
                ss << "\"request\":" << current_task.input_json << ",";
                // A quick and dirty way to escape quotes in the error message
                std::string error_msg = error->error_message;
                std::string escaped_error_msg;
                for (char c : error_msg)
                {
                    if (c == '\"')
                    {
                        escaped_error_msg += "\\\"";
                    }
                    else
                    {
                        escaped_error_msg += c;
                    }
                }
                ss << "\"status\":\"error\",";
                ss << "\"response\":\"" << escaped_error_msg << "\"";
            }
            ss << "}";

            std::stringstream filename_ss;
            filename_ss << std::setw(LOG_ID_PADDING) << std::setfill('0') << current_task.id
                        << "_" << current_task.command_name << ".json";
            std::ofstream log_file(log_path_ / filename_ss.str());
            log_file << ss.str();

            result_repo_->store_result(current_task.id, std::move(result));
            spdlog::info("Result for command ID {} stored.", current_task.id);
        }
    }

    std::string CommandProcessor::resolve_refs(const std::string &input_json, const std::string &command_name_of_current_cmd, uint64_t current_cmd_id)
    {
        spdlog::debug("Starting reference resolution for command {}: {}", current_cmd_id, input_json);

        auto resolved_json = input_json;

        while (true) {
            const std::regex ref_regex(R"(\$ref:(?:cmd\[(\d+)\]|(latest)|prev\[(\d+)\])\.(\w+))");
            const std::regex file_ref_regex(R"(\$ref:([a-zA-Z0-9_.-]+\.json)#([/\w.-]+))");

            std::smatch match;
            bool found_ref = false;

            if (std::regex_search(resolved_json, match, ref_regex)) {
                found_ref = true;
                const std::string full_match_str = match[0].str();
                const std::string cmd_id_str = match[1].str();
                const std::string latest_str = match[2].str();
                const std::string prev_n_str = match[3].str();
                const std::string member_name = match[4].str();

                std::optional<uint64_t> cmd_id_opt;

                if (!cmd_id_str.empty())
                {
                    cmd_id_opt = std::stoull(cmd_id_str);
                    spdlog::debug("Found reference by ID: {}, cmd_id={}, member={}", full_match_str, *cmd_id_opt, member_name);
                }
                else if (!latest_str.empty())
                {
                    cmd_id_opt = result_repo_->get_latest_result_id(current_cmd_id);
                    if (!cmd_id_opt)
                    {
                        throw std::runtime_error("Reference 'latest' found, but no previous command exists.");
                    }
                    spdlog::debug("Found reference to latest command: {}, resolved cmd_id={}, member={}", full_match_str, *cmd_id_opt, member_name);
                }
                else if (!prev_n_str.empty())
                {
                    size_t n = std::stoull(prev_n_str);
                    if (n == 0)
                    {
                        throw std::runtime_error("Reference 'prev[0]' is invalid. Index must be 1 or greater.");
                    }
                    cmd_id_opt = result_repo_->get_nth_latest_result_id(n, current_cmd_id);
                    if (!cmd_id_opt)
                    {
                        throw std::runtime_error("Reference 'prev[" + std::to_string(n) + "]' not found.");
                    }
                    spdlog::debug("Found reference to {} previous command: {}, resolved cmd_id={}, member={}", n, full_match_str, *cmd_id_opt, member_name);
                }

                if (!cmd_id_opt)
                {
                    throw std::runtime_error("Could not resolve reference: " + full_match_str);
                }
                const uint64_t cmd_id = *cmd_id_opt;

                auto result = result_repo_->get_result(cmd_id);
                if (!result)
                {
                    throw std::runtime_error("Referenced command with ID " + std::to_string(cmd_id) + " not found.");
                }
                spdlog::debug("Successfully retrieved result for command ID {}", cmd_id);

                auto success_result = std::get_if<SuccessResult>(&(*result));
                if (!success_result)
                {
                    throw std::runtime_error("Referenced command with ID " + std::to_string(cmd_id) + " failed.");
                }

                yyjson_doc *doc = yyjson_read(success_result->output_json.c_str(), success_result->output_json.length(), 0);
                if (!doc)
                {
                    throw std::runtime_error("Failed to parse output JSON for command ID " + std::to_string(cmd_id));
                }
                spdlog::debug("Successfully parsed referenced JSON document.");

                yyjson_val *root = yyjson_doc_get_root(doc);
                if (!yyjson_is_obj(root))
                {
                    yyjson_doc_free(doc);
                    throw std::runtime_error("Output of command ID " + std::to_string(cmd_id) + " is not a JSON object.");
                }

                yyjson_val *member_val = yyjson_obj_get(root, member_name.c_str());
                if (!member_val)
                {
                    yyjson_doc_free(doc);
                    throw std::runtime_error("Member '" + member_name + "' not found in output of command ID " + std::to_string(cmd_id));
                }
                spdlog::debug("Successfully found member '{}' in JSON.", member_name);

                const char *member_json_c_str = yyjson_val_write(member_val, 0, NULL);
                if (!member_json_c_str)
                {
                    yyjson_doc_free(doc);
                    throw std::runtime_error("Failed to write member '" + member_name + "' to string (yyjson_val_write returned null).");
                }
                spdlog::debug("Successfully created member JSON string.");
                std::string member_json(member_json_c_str);
                free((void *)member_json_c_str);
                yyjson_doc_free(doc);

                std::string target_to_replace = "\"" + full_match_str + "\"";
                size_t pos = resolved_json.find(target_to_replace);

                if (pos == std::string::npos)
                {
                    target_to_replace = full_match_str;
                    pos = resolved_json.find(target_to_replace);
                }

                if (pos != std::string::npos) {
                    resolved_json.replace(pos, target_to_replace.length(), member_json);
                } else {
                    spdlog::warn("Could not find the reference string '{}' for replacement.", full_match_str);
                }

            } else if (std::regex_search(resolved_json, match, file_ref_regex)) {
                found_ref = true;
                const std::string full_match_str = match[0].str();
                const std::string filename = match[1].str();
                const std::string json_pointer = match[2].str();

                spdlog::debug("Found file reference: filename={}, pointer={}", filename, json_pointer);

                std::optional<uint64_t> param_id_opt;
                std::optional<SuccessResult> param_result_opt;

                auto all_results = result_repo_->get_all_results();
                for (const auto& [id, result] : all_results) {
                    if (const auto* success = std::get_if<SuccessResult>(&result)) {
                        if (success->command_name == filename) {
                            param_id_opt = id;
                            param_result_opt = *success;
                            break;
                        }
                    }
                }

                if (!param_id_opt) {
                    throw std::runtime_error("Referenced parameter file '" + filename + "' not found or not loaded.");
                }

                const auto& param_result = *param_result_opt;

                yyjson_doc* doc = yyjson_read(param_result.output_json.c_str(), param_result.output_json.length(), 0);
                if (!doc) {
                    throw std::runtime_error("Failed to parse JSON for parameter file '" + filename + "'");
                }

                yyjson_val* root = yyjson_doc_get_root(doc);
                yyjson_val* value_at_pointer = yyjson_ptr_get(root, json_pointer.c_str());

                if (!value_at_pointer) {
                    yyjson_doc_free(doc);
                    throw std::runtime_error("JSON Pointer '" + json_pointer + "' not found in parameter file '" + filename + "'");
                }

                const char* value_json_c_str = yyjson_val_write(value_at_pointer, 0, NULL);
                if (!value_json_c_str) {
                    yyjson_doc_free(doc);
                    throw std::runtime_error("Failed to write value from JSON pointer to string.");
                }

                std::string value_json(value_json_c_str);
                free((void*)value_json_c_str);
                yyjson_doc_free(doc);

                std::string target_to_replace = "\"" + full_match_str + "\"";
                size_t pos = resolved_json.find(target_to_replace);
                if (pos == std::string::npos) {
                    target_to_replace = full_match_str;
                    pos = resolved_json.find(target_to_replace);
                }

                if (pos != std::string::npos) {
                    resolved_json.replace(pos, target_to_replace.length(), value_json);
                } else {
                    spdlog::warn("Could not find the reference string '{}' for replacement.", full_match_str);
                }
            }

            if (!found_ref) {
                break;
            }
        }

        const size_t max_display_length = 256;
        std::string resolved_json_for_display = resolved_json;

        if (resolved_json.size() > max_display_length)
        {
            resolved_json_for_display = resolved_json.substr(0,max_display_length) + "...";
            spdlog::debug("json trancated {} to {}", resolved_json.size(), resolved_json_for_display.size());
        }

        spdlog::debug("Reference resolution finished. Final JSON: {}", resolved_json_for_display);
        return resolved_json;
    }

    uint64_t CommandProcessor::add_to_queue(const std::string &command_name, const std::string &input_json)
    {
        const uint64_t id = next_command_id_++;
        log_unresolved_command(id, command_name, input_json, command_history_path_);
        std::function<CommandResult()> task_logic;

        if (auto it = cartridge_manager.find(command_name); it != cartridge_manager.end())
        {
            task_logic = [this, handler = it->second.handler, input_json, command_name, id]
            {
                try
                {

                    const std::string resolved_input_json = this->resolve_refs(input_json, command_name, id);
                    CommandResult result = handler(resolved_input_json);
                    if (auto *success = std::get_if<SuccessResult>(&result))
                    {
                        success->unresolved_input_json = input_json;
                        success->resolved_input_json = resolved_input_json;
                    }
                    return result;
                }
                catch (const std::exception &e)
                {
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
            command_queue_.emplace(CommandTask{id, command_name, input_json, std::move(task_logic)});
        }
        condition_.notify_one();

        spdlog::info("Command '{}' with ID {} added to the queue. Input: {}", command_name, id, input_json);
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

    std::map<std::string, std::string> CommandProcessor::get_input_schema(const std::string &command_name) const
    {
        auto it = cartridge_manager.find(command_name);
        if (it != cartridge_manager.end())
        {
            return it->second.input_schema.arg_names_to_type;
        }
        return {};
    }

    void CommandProcessor::load_result_from_log(const std::string &json_content)
    {
        struct LogFileFormat
        {
            rfl::Field<"id", uint64_t> id;
            rfl::Field<"command", std::string> command;
            rfl::Field<"status", std::string> status;
            rfl::Field<"request", rfl::Generic> request;
            rfl::Field<"response", rfl::Generic> response;
            rfl::Field<"schema", std::optional<std::map<std::string, std::string>>> schema;
        };

        auto parsed_log = rfl::json::read<LogFileFormat>(json_content);
        if (!parsed_log)
        {
            spdlog::error("Failed to parse log file for loading: {}", parsed_log.error().what());
            return;
        }

        parsed_log->command.value() = parsed_log->command.value() + "(Loaded)";

        const auto &log = *parsed_log;
        spdlog::info("request: {}", rfl::json::write(log.request()));

        spdlog::info("response: {}", rfl::json::write(log.response()));

        if (log.status() == "success" && log.schema())
        {
            SuccessResult success;
            success.command_name = log.command();
            success.output_json = rfl::json::write(log.response());
            success.output_schema = *log.schema();

            // input_raw and output_raw are left empty as they are not needed for tracing.

            result_repo_->store_result(log.id(), std::move(success));
            spdlog::info("Successfully loaded result for command ID {} from log.", log.id());
        }
        else if (log.status() == "error")
        {
            ErrorResult error;
            // The response for an error is a simple string.
            auto str_result = rfl::to_string(log.response());
            if (str_result)
            {
                error.error_message = *str_result;
            }
            else
            {
                error.error_message = "Could not parse error message from log.";
            }
            result_repo_->store_result(log.id(), std::move(error));
            spdlog::info("Successfully loaded error result for command ID {} from log.", log.id());
        }
        else
        {
            spdlog::warn("Log for command ID {} could not be loaded. A 'success' status requires a 'schema' field, which was not found. This may be an old log file format.", log.id());
        }
    }

} // namespace MITSU_Domoe


uint64_t MITSU_Domoe::CommandProcessor::load_parameters_from_file(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        spdlog::error("Parameter file not found: {}", file_path.string());
        return 0; // Or throw an exception
    }

    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        spdlog::error("Failed to open parameter file: {}", file_path.string());
        return 0;
    }

    std::string json_content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());

    // We can parse the json to make sure it's valid.
    const auto parsed = rfl::json::read<rfl::Generic>(json_content);
    if (!parsed) {
        spdlog::error("Failed to parse parameter file: {}", file_path.string());
        return 0;
    }

    SuccessResult result_capsule;
    result_capsule.command_name = file_path.filename().string();
    result_capsule.output_json = json_content;
    result_capsule.output_raw = *parsed; // Store the parsed object

    const auto new_id = next_command_id_++;
    result_repo_->store_result(new_id, result_capsule);

    spdlog::info("Loaded parameters from '{}' with ID {}", file_path.string(), new_id);

    return new_id;
}
