#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include <rfl.hpp>
#include <vector>
#include <thread>
#include <chrono>

class BIGprocess_mock_cartridge
{
public:
    
    // 1. 入力用の構造体を定義
    struct Input
    {
        unsigned int time_to_process;
    };

    // 2. 出力用の構造体を定義 (複数の値を返す例)
    struct Output
    {
        unsigned int time_slept;
        std::string message;
    };
    static inline const std::string command_name="BIGprocess_mock";
    static inline const std::string description = "Mock cartridge of long process";
    // 3. 処理本体を実装
    Output
    execute(const Input &input) const
    {
        std::cout << "\n--- Executing BIGprocess_mock_cartridge (will sleep for " << input.time_to_process << " seconds) ---" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(input.time_to_process));

        std::cout << "--- BIGprocess_mock_cartridge finished sleeping ---" << std::endl;

        // Output構造体に結果を詰めて返す
        return Output{
            .time_slept = input.time_to_process,
            .message = "Successfully slept"};
    }
};

// コンセプトを満たしていることをコンパイラにチェックさせる (任意)
static_assert(MITSU_Domoe::Cartridge<BIGprocess_mock_cartridge>);
