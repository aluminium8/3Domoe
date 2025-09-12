#pragma once

#include "ICartridge.hpp"
#include "ResultRepository.hpp"

#include <rfl/json.hpp>
#include <spdlog/spdlog.h>
#include <atomic>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <any>
#include <regex>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "rfl/thirdparty/yyjson.h"
#endif

namespace MITSU_Domoe
{

    // (中身は前回と同じ。実装はヘッダに残す)
    class CommandProcessor
    {
    public:
        CommandProcessor(std::shared_ptr<ResultRepository> repo, const std::filesystem::path& log_path);
        ~CommandProcessor();

        template <Cartridge C>
        void register_cartridge(C cartridge)
        {

            std::string command_name=cartridge.command_name;
            spdlog::debug("Registering command: {}", command_name);
            if(cartridge_manager.count(command_name)){
                spdlog::warn("Cartridge '{}' is being overwritten.", command_name);
            }


            cartridge_manager[command_name].description = cartridge.description;

            for (const auto &f : rfl::fields<typename C::Input>())
            {
                cartridge_manager[command_name].input_schema.arg_names_to_type[f.name()] = f.type();
            }

            for (const auto &f : rfl::fields<typename C::Output>())
            {
                spdlog::debug("Registering output for command '{}': field_name='{}', field_type='{}'", command_name, f.name(), f.type());
                cartridge_manager[command_name].output_schema[f.name()] = f.type();
            }


            cartridge_manager[command_name].handler = [cartridge, command_name, this, output_schema = cartridge_manager[command_name].output_schema](const std::string &input_json) -> CommandResult
            {
                try
                {
                    auto input_obj = rfl::json::read<typename C::Input>(input_json);
                    if (!input_obj)
                    {
                        return ErrorResult{"Input JSON deserialization failed: " +
                                           input_obj.error().what()};
                    }

                    typename C::Output output_obj = cartridge.execute(*input_obj);

                    SuccessResult result_capsule;
                    result_capsule.input_raw = input_obj;
                    result_capsule.output_json = rfl::json::write(output_obj);
                    result_capsule.output_raw = output_obj;
                    result_capsule.command_name = command_name;
                    result_capsule.output_schema = output_schema;

                    return result_capsule;
                }
                catch (const std::exception &e)
                {
                    return ErrorResult{
                        "An exception occurred during command execution: " +
                        std::string(e.what())};
                }
            };



            spdlog::info("Cartridge registered: {}", command_name);
        }

        uint64_t add_to_queue(const std::string &command_name, const std::string &input_json);
        uint64_t load_json_from_file(const std::filesystem::path& file_path);
        void load_result_from_log(const std::string& json_content);
        void start();
        void stop();

        std::map<std::string, std::string> get_cartridge_schemas() const;
        std::vector<std::string> get_command_names() const;
        std::map<std::string, std::string> get_input_schema(const std::string& command_name) const;

    private:
        void worker_loop();
        // CommandProcessorの内部クラスとして定義すると良い
        struct Input_Schema
        {
            std::map<std::string, std::string> arg_names_to_type;
        };
        struct Cartridge_info
        {
            std::function<CommandResult(const std::string &input_json)> handler;
            std::function<CommandResult(const std::any &source_output, const std::string &member_name)> extractor;
            Input_Schema input_schema;
            std::map<std::string, std::string> output_schema;
            std::string description;
        };

        std::string resolve_refs(const std::string &input_json, const std::string& command_name_of_current_cmd, uint64_t current_cmd_id);


        std::map<std::string, Cartridge_info> cartridge_manager;

        struct CommandTask {
            uint64_t id;
            std::string command_name;
            std::string input_json;
            std::function<CommandResult()> task;
        };
        std::queue<CommandTask> command_queue_;

        std::atomic<uint64_t> next_command_id_{1};
        std::shared_ptr<ResultRepository> result_repo_;
        std::filesystem::path log_path_;
        std::filesystem::path command_history_path_;

        std::thread worker_thread_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_flag_{false};
    };

} // namespace MITSU_Domoe
