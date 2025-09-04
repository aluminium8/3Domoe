#pragma once

#include "ICartridge.hpp"
#include "ResultRepository.hpp"

#include <rfl/json.hpp>
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
        CommandProcessor(std::shared_ptr<ResultRepository> repo);
        ~CommandProcessor();

        template <Cartridge C>
        void register_cartridge(C cartridge)
        {

            std::string command_name=cartridge.command_name;
            std::cout<<"command_name:"<< command_name<<std::endl;
            if(cartridge_manager.count(command_name)){
                std::cout<<"WARN: cartridge will be overwritten"<<std::endl;
            }


            cartridge_manager[command_name].description = cartridge.description;

            for (const auto &f : rfl::fields<typename C::Input>())
            {
                cartridge_manager[command_name].input_schema.arg_names_to_type[f.name()] = f.type();
            }

            for (const auto &f : rfl::fields<typename C::Output>())
            {
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



            std::cout << "Cartridge registered: " << command_name << std::endl;
        }

        uint64_t add_to_queue(const std::string &command_name, const std::string &input_json);
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

        std::string resolve_refs(const std::string &input_json, const std::string& command_name_of_current_cmd);


        std::map<std::string, Cartridge_info> cartridge_manager;
        // std::map<std::string, std::function<CommandResult(const std::string&)>> handlers_;
        std::queue<std::pair<uint64_t, std::function<CommandResult()>>> command_queue_;
        std::atomic<uint64_t> next_command_id_{1};
        std::shared_ptr<ResultRepository> result_repo_;

        std::thread worker_thread_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_flag_{false};
    };

} // namespace MITSU_Domoe
