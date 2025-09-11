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

    CommandProcessor::CommandProcessor(std::shared_ptr<ResultRepository> repo, const std::filesystem::path &log_path)
        : result_repo_(std::move(repo)), log_path_(log_path) {}

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
            ss << "\"request\":" << current_task.input_json << ",";

            if (const auto *success = std::get_if<SuccessResult>(&result))
            {
                ss << "\"status\":\"success\",";
                ss << "\"response\":" << success->output_json << ",";
                ss << "\"schema\":" << rfl::json::write(success->output_schema);
            }
            else if (const auto *error = std::get_if<ErrorResult>(&result))
            {
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

    std::string CommandProcessor::resolve_refs(const std::string &input_json, const std::string &command_name_of_current_cmd)
    {
        spdlog::debug("Starting reference resolution for: {}", input_json);

        const std::regex ref_regex(R"(\$ref:cmd\[(\d+)\]\.(\w+))");
        auto resolved_json = input_json;

        const std::regex key_finder_regex(R"("\"(\w+)\"\s*:\s*\"?\$ref:cmd)"); // MODIFIED: `"`をオプショナルに
        std::smatch key_match;
        std::string key_of_ref_field;
        if (std::regex_search(input_json, key_match, key_finder_regex) && key_match.size() > 1)
        {
            key_of_ref_field = key_match[1].str();
        }

        std::smatch match;
        // NOTE: この実装はJSON内に複数の$refがある場合、正しく動作しない可能性がある
        //       regex_searchは文字列の先頭から検索するため、置換によって文字列長が変わると
        //       後続の検索に影響が出る。今回は$refが1つと仮定する。
        if (std::regex_search(resolved_json, match, ref_regex))
        {
            const std::string full_match_str = match[0].str();
            const uint64_t cmd_id = std::stoull(match[1].str());
            const std::string member_name = match[2].str();
            spdlog::debug("Found reference: {}, cmd_id={}, member={}", full_match_str, cmd_id, member_name);

            // Type Checking Logic
            if (!key_of_ref_field.empty())
            {
                // (この部分は変更なし)
                spdlog::debug("Performing type check for field '{}'...", key_of_ref_field);
                const auto &current_cartridge_info = cartridge_manager.at(command_name_of_current_cmd);
                const auto &expected_type = current_cartridge_info.input_schema.arg_names_to_type.at(key_of_ref_field);

                auto referenced_result = result_repo_->get_result(cmd_id);
                if (referenced_result && std::holds_alternative<SuccessResult>(*referenced_result))
                {
                    const auto &success_result = std::get<SuccessResult>(*referenced_result);
                    const auto &actual_type = success_result.output_schema.at(member_name);
                    if (expected_type != actual_type)
                    {
                        throw std::runtime_error("Type mismatch for field '" + key_of_ref_field + "'. Expected " + expected_type + " but got " + actual_type + ".");
                    }
                    spdlog::debug("Type check passed. Expected '{}', got '{}'", expected_type, actual_type);
                }
            }

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

            // ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼
            // ▼▼▼ ここからが修正箇所 ▼▼▼
            // ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼

            // 最初にダブルクオーテーション付きの参照文字列を検索
            std::string target_to_replace = "\"" + full_match_str + "\"";
            size_t pos = resolved_json.find(target_to_replace);

            // 見つからなかった場合、ダブルクオーテーションなしの参照文字列を検索
            if (pos == std::string::npos)
            {
                target_to_replace = full_match_str; // 検索対象をクオートなしに変更
                pos = resolved_json.find(target_to_replace);
                if (pos != std::string::npos)
                {
                    spdlog::debug("Found unquoted reference, will replace it.");
                }
            }
            else
            {
                spdlog::debug("Found quoted reference, will replace it.");
            }

            if (pos != std::string::npos)
            {
                resolved_json.replace(pos, target_to_replace.length(), member_json);
                spdlog::debug("Replacement complete.");
            }
            else
            {
                spdlog::warn("Could not find the reference string '{}' (with or without quotes) for replacement.", full_match_str);
            }

            // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲
            // ▲▲▲ ここまでが修正箇所 ▲▲▲
            // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲
        }

        spdlog::debug("Reference resolution finished. Final JSON: {}", resolved_json);
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
                try
                {
                    const std::string resolved_input_json = this->resolve_refs(input_json, command_name);
                    return handler(resolved_input_json);
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
