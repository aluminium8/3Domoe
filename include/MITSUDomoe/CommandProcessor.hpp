#pragma once

#include "ICartridge.hpp"
#include "ResultRepository.hpp"
#include <rfl/toml.hpp>
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

namespace MITSU_Domoe {

// (中身は前回と同じ。実装はヘッダに残す)
class CommandProcessor {
public:
    CommandProcessor(std::shared_ptr<ResultRepository> repo);

 template <Cartridge C>
    void register_cartridge(const std::string &command_name, C cartridge)
    {
        cartridge_manager[command_name].handler = [cartridge, command_name,this](const std::string &input_toml) -> CommandResult
        {
            try
            {
                auto input_obj = rfl::toml::read<typename C::Input>(input_toml);
                if (!input_obj)
                {
                    return ErrorResult{"Input TOML deserialization failed: " +
                                       input_obj.error().what()};
                }


                typename C::Output output_obj = cartridge.execute(*input_obj);

                SuccessResult result_capsule;
                result_capsule.input_raw = input_obj;
                result_capsule.output_toml = rfl::toml::write(output_obj);
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

        cartridge_manager[command_name].description = cartridge.description;

        auto nt = rfl::to_named_tuple(typename C::Input{});
        std::map<std::string, std::string> schema;
        nt.apply([&](const auto& f) {
            schema[std::string(f.name())] = typeid(typename std::remove_pointer_t<decltype(f.value())>).name();
        });
        cartridge_manager[command_name].input_schema = rfl::json::write(schema);

        std::cout << "Cartridge registered: " << command_name << std::endl;
    }

    uint64_t add_to_queue(const std::string& command_name, const std::string& input_toml);
    void process_queue();

    std::map<std::string, std::string> get_cartridge_schemas() const;

private:
    // CommandProcessorの内部クラスとして定義すると良い
    struct Cartridge_info
    {
        // 1. コマンドを実行するための関数 (従来のhandlers_の中身)
        std::function<CommandResult(const std::string &input_toml)> handler;

        // 2. このコマンドの出力からメンバーを抽出するための関数
        std::function<CommandResult(const std::any &source_output, const std::string &member_name)> extractor;

        // 3. 将来のための拡張
        std::string description; // GUIに表示するコマンドの説明
        std::string input_schema;
    };

    std::map<std::string, Cartridge_info> cartridge_manager;
    //std::map<std::string, std::function<CommandResult(const std::string&)>> handlers_;
    std::queue<std::pair<uint64_t, std::function<CommandResult()>>> command_queue_;
    std::atomic<uint64_t> next_command_id_{1};
    std::shared_ptr<ResultRepository> result_repo_;
};

} // namespace MITSU_Domoe
