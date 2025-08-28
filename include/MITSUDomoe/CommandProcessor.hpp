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

namespace MITSU_Domoe
{

    // (中身は前回と同じ。実装はヘッダに残す)
    class CommandProcessor
    {
    public:
        CommandProcessor(std::shared_ptr<ResultRepository> repo);

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
            for (const auto &arg : cartridge_manager[command_name].input_schema.arg_names_to_type)
            {
                std::cout << "name: " << arg.first << ", type: " << arg.second << std::endl;
            }


            cartridge_manager[command_name].handler = [cartridge, command_name, this](const std::string &input_json) -> CommandResult
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
        void process_queue();

        std::map<std::string, std::string> get_cartridge_schemas() const;

    private:
        // CommandProcessorの内部クラスとして定義すると良い
        struct Input_Schema
        {
            std::map<std::string, std::string> arg_names_to_type;
        };
        struct Cartridge_info
        {
            // 1. コマンドを実行するための関数 (従来のhandlers_の中身)
            std::function<CommandResult(const std::string &input_json)> handler;

            // 2. このコマンドの出力からメンバーを抽出するための関数
            std::function<CommandResult(const std::any &source_output, const std::string &member_name)> extractor;
            Input_Schema input_schema;

            std::string description; // GUIに表示するコマンドの説明
        };

        std::map<std::string, Cartridge_info> cartridge_manager;
        // std::map<std::string, std::function<CommandResult(const std::string&)>> handlers_;
        std::queue<std::pair<uint64_t, std::function<CommandResult()>>> command_queue_;
        std::atomic<uint64_t> next_command_id_{1};
        std::shared_ptr<ResultRepository> result_repo_;
    };

} // namespace MITSU_Domoe
