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
        unsigned int input_mesh_id;
    };

    // 2. 出力用の構造体を定義 (複数の値を返す例)
    struct Output
    {
        unsigned int generated_point_cloud_id;
        std::string message;
    };
    std::string command_name="BIGprocess_mock";
    std::string description = "Mock cartridge of long process";
    // 3. 処理本体を実装
    Output
    execute(const Input &input) const
    {
        std::cout << "\n--- Executing BIGprocess_mock_cartridge (by sleep) ---" << std::endl;
        std::cout << "  Processing Mesh ID: " << input.input_mesh_id << std::endl;

        // 3秒間スリープ
        std::cout << "sleep 1 sec..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "sleep finished" << std::endl;

        // Output構造体に結果を詰めて返す
        return Output{
            .generated_point_cloud_id = input.input_mesh_id + 1000,
            .message = "Successfully sleeped"};
    }
};

// コンセプトを満たしていることをコンパイラにチェックさせる (任意)
static_assert(MITSU_Domoe::Cartridge<BIGprocess_mock_cartridge>);
