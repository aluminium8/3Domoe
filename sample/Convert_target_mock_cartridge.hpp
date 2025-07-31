#pragma once

#include "MITSUDomoe/convert_target_types.hpp"
#include "MITSUDomoe/ICartridge.hpp"
#include <rfl.hpp>
#include <vector>
#include <thread>
#include <chrono>

class Convert_target_mock_cartridge
{
public:
    // 1. 入力用の構造体を定義
    struct Input
    {
        Mock_mesh_id mock_mesh_id;
    };

    // 2. 出力用の構造体を定義 (複数の値を返す例)
    struct Output
    {

        std::string message;
    };
    std::string description="Mock cartridge of long process";
        // 3. 処理本体を実装
        Output
        execute(const Input &input) const
    {
   
        // Output構造体に結果を詰めて返す
        return Output{
            .message = "(cartridge has no function)"};
    }
};

// コンセプトを満たしていることをコンパイラにチェックさせる (任意)
static_assert(Cartridge<Convert_target_mock_cartridge>);