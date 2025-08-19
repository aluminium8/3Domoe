#pragma once

#include <concepts>
#include <string>
#include <variant>
#include <any>

namespace MITSU_Domoe {

// 全てのカートリッジが満たすべき規約(コンセプト)
template<typename T>
concept Cartridge = requires(T cartridge, const typename T::Input& input) {
    // Input型とOutput型が定義されていること
    typename T::Input;
    typename T::Output;

    // Output execute(const Input&) というシグネチャのメソッドを持つこと
    { cartridge.execute(input) } -> std::same_as<typename T::Output>;
    // std::stringに変換可能であることを要求
    { T::command_name } -> std::convertible_to<std::string>;
    { T::description } -> std::convertible_to<std::string>;
    
};

// コマンド実行結果の汎用的な表現
// 成功時はシリアライズされたOutput(JSON文字列)を、失敗時はエラー情報を保持
struct SuccessResult {
    std::string command_name;
    std::string output_json;
    std::any input_raw;
    std::any output_raw;
};
struct ErrorResult {
    std::string error_message;
};
using CommandResult = std::variant<SuccessResult, ErrorResult>;

} // namespace MITSU_Domoe
