#pragma once

#include <rfl/toml.hpp>
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <any>

#include "ICartridge.hpp"
#include "ResultRepository.hpp"
#include "MITSUDomoe/convert_target_types.hpp"

struct virtual_input
{
    Mock_mesh mock_mesh;
};

// (中身は前回と同じ。実装はヘッダに残す)
class CommandProcessor
{
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

                if (command_name == "Convert_target_mock_cartridge")
                {
                    std::cout << "convert_check!!!!!!!!!!!!!" << std::endl;

                    virtual_input v=this->ac.convert<virtual_input>(input_obj);
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
        std::cout << "Cartridge registered: " << command_name << std::endl;
    }

    uint64_t add_to_queue(const std::string &command_name, const std::string &input_toml);
    void process_queue();

private:
    class AssetConverter
    {
    private:
        template <rfl::internal::StringLiteral str>
        static consteval auto remove_id_suffix()
        { /* ... 実装は省略 ... */
            constexpr auto sv = str.string_view();
            constexpr auto new_sv = sv.substr(0, sv.size() - 3);
            constexpr auto new_size = new_sv.size();
            std::array<char, new_size + 1> arr{};
            for (size_t i = 0; i < new_size; ++i)
                arr[i] = new_sv[i];
            arr[new_size] = '\0';
            return rfl::internal::StringLiteral<new_size + 1>(arr);
        }

    public:
        AssetConverter() {}
        Mock_mesh operator()(const Mock_mesh_id &meshid) const { return Mock_mesh{std::to_string(meshid.id)}; }

        template <typename To, typename From>
        To convert(const From &from) const
        {
            const auto from_nt = rfl::to_named_tuple(from);
            const auto to_nt = from_nt.transform([&](auto &&field)
                                                 {
            using FieldType = std::remove_cvref_t<decltype(field)>;
            constexpr auto name_literal = FieldType::name_;
            if constexpr (name_literal.string_view().ends_with("_id")) {
                constexpr auto new_name_literal = remove_id_suffix<name_literal>();
                auto new_value = (*this)(field.get());
                return rfl::make_field<new_name_literal>(new_value);
            } else {
                return std::forward<decltype(field)>(field);
            } });
            return rfl::from_named_tuple<To>(to_nt);
        }
    };

    // CommandProcessorの内部クラスとして定義すると良い
    struct Cartridge_info
    {
        // 1. コマンドを実行するための関数 (従来のhandlers_の中身)
        std::function<CommandResult(const std::string &input_toml)> handler;

        // 2. このコマンドの出力からメンバーを抽出するための関数
        std::function<CommandResult(const std::any &source_output, const std::string &member_name)> extractor;

        // 3. 将来のための拡張
        std::string description; // GUIに表示するコマンドの説明
    };
    struct Command_history
    {
    };

    std::map<std::string, Cartridge_info> cartridge_manager;

    // std::map<std::string, std::function<CommandResult(const std::string &)>> handlers_;
    std::queue<std::pair<uint64_t, std::function<CommandResult()>>> command_queue_;
    std::atomic<uint64_t> next_command_id_{0};
    std::shared_ptr<ResultRepository> result_repo_;
    AssetConverter ac;
};