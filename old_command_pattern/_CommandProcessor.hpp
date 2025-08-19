#pragma once

#include "_ICartridge.hpp"
#include <rfl/json.hpp>
#include <future>
#include <map>
#include <string>
#include <functional>


class CommandProcessor
{
public:
    // Cartridgeコンセプトを満たすオブジェクトを登録する
    template <Cartridge C> // C++20の機能
    void register_cartridge(const std::string &command_name, C cartridge)
    {
        // 登録する処理の本体。「入力jsonを受け取り、CommandResultを返す関数」である。
        handlers_[command_name] = [cartridge](const std::string &input_json) -> CommandResult
        {
            // ★★ここから修正★★
            try
            {
                // 1. 入力jsonをカートリッジのInput型にデシリアライズ

                auto input_obj = rfl::json::read<typename C::Input>(input_json);
                if (!input_obj)
                {
                    return ErrorResult{"Input json deserialization failed: " + input_obj.error().what()};
                }
                // 2. カートリッジの処理を実行

                typename C::Output output_obj = cartridge.execute(*input_obj);
                // 3. 結果のOutputオブジェクトを出力jsonにシリアライズ

                return SuccessResult{rfl::json::write(output_obj)};
            }
            catch (const std::exception &e)
            {
                // パース中の例外などをここでキャッチする
                return ErrorResult{"An exception occurred during command execution: " + std::string(e.what())};
            }
            // ★★ここまで修正★★
        };
        std::cout << "Cartridge registered: " << command_name << std::endl;
    }

    // コマンド名と入力jsonから、実行タスクとfutureを作成する
    std::pair<std::packaged_task<CommandResult()>, std::future<CommandResult>>
    create_task(const std::string &command_name, const std::string &input_json)
    {

        std::function<CommandResult()> task_logic;
        if (auto it = handlers_.find(command_name); it != handlers_.end())
        {
            // 登録されたハンドラと入力jsonをキャプチャ
            task_logic = [handler = it->second, input_json]
            {
                return handler(input_json);
            };
        }
        else
        {
            task_logic = [command_name]
            {
                return ErrorResult{"Error: Command '" + command_name + "' not found."};
            };
        }

        std::packaged_task<CommandResult()> task(std::move(task_logic));
        auto future = task.get_future();
        return {std::move(task), std::move(future)};
    }

private:
    std::map<std::string, std::function<CommandResult(const std::string &)>> handlers_;
};