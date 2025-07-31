#pragma once

#include <concepts>
#include <string>
#include <variant>


// 全てのカートリッジが満たすべき規約(コンセプト)
template<typename T>
concept Cartridge = requires(T cartridge, const typename T::Input& input) {
    // Input型とOutput型が定義されていること
    typename T::Input;
    typename T::Output;

    // Output execute(const Input&) というシグネチャのメソッドを持つこと
    { cartridge.execute(input) } -> std::same_as<typename T::Output>;
};

// コマンド実行結果の汎用的な表現
// 成功時はシリアライズされたOutput(TOML文字列)を、失敗時はエラー情報を保持
struct SuccessResult {
    std::string output_toml;
    std::any output_raw;
};
struct ErrorResult {
    std::string error_message;
};
using CommandResult = std::variant<SuccessResult, ErrorResult>;