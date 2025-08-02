#include "rfl_eigen_serdes.hpp" // あなたが作成したヘッダ
#include <iostream>
#include "rfl/json.hpp"
#include "rfl/toml.hpp"
// 簡単なテスト用の構造体
struct MyData {
    rfl::Field<"matrix", Eigen::Matrix<double, 2, 3, Eigen::RowMajor>> mat;
};

int main() {
    // --- 1. JSONでの成功ケース ---
    std::cout << "--- 1. Successful Deserialization from JSON ---" << std::endl;
    std::string json_ok = R"({
        "storageOrder": "RowMajor",
        "data": [
            [1.0, 2.0, 3.0],
            [4.0, 5.0, 6.0]
        ]
    })";

    rfl::json::read<rfl::Generic>(json_ok)
    .and_then([](const auto& g){
        return rfl::from_generic<Eigen::Matrix<double, 2, 3, Eigen::RowMajor>>(g);
    })
    .transform([](const auto& m){
        std::cout << "Successfully converted from rfl::Generic (JSON):\n" << m << std::endl;
    })
    .or_else([](const auto& err){
        std::cout << "Failed to convert from rfl::Generic (JSON): " << err.what() << std::endl;
        return rfl::Result<Eigen::Matrix<double, 2, 3, Eigen::RowMajor>>(err);
    });

    std::cout << std::endl;

    // --- 2. JSONでの失敗ケース (検証ロジック) ---
    std::cout << "--- 2. Failed Deserialization from JSON (Validation Check) ---" << std::endl;
    std::string json_fail = R"({
        "storageOrder": "ColMajor",
        "data": [ [1, 2, 3], [4, 5, 6] ]
    })";

    rfl::json::read<rfl::Generic>(json_fail)
    .and_then([](const auto& g){
        return rfl::from_generic<Eigen::Matrix<double, 2, 3, Eigen::RowMajor>>(g);
    })
    .transform([](const auto& m){
        std::cout << "Successfully converted from rfl::Generic (JSON):\n" << m << std::endl;
    })
    .or_else([](const auto& err){
        std::cout << "Failed to convert from rfl::Generic (JSON): " << err.what() << std::endl;
        return rfl::Result<Eigen::Matrix<double, 2, 3, Eigen::RowMajor>>(err);
    });

    std::cout << std::endl;


    // ▼▼▼ 追加したTOMLの例 ▼▼▼
    // --- 3. TOMLでの成功ケース ---
    std::cout << "--- 3. Successful Deserialization from TOML ---" << std::endl;
    std::string toml_ok = R"(
        storageOrder = "RowMajor"
        data = [
            [10.0, 20.0, 30.0],
            [40.0, 50.0, 60.0]
        ]
    )";

    rfl::toml::read<rfl::Generic>(toml_ok)
    .and_then([](const auto& g){
        return rfl::from_generic<Eigen::Matrix<double, 2, 3, Eigen::RowMajor>>(g);
    })
    .transform([](const auto& m){
        std::cout << "Successfully converted from rfl::Generic (TOML):\n" << m << std::endl;
    })
    .or_else([](const auto& err){
        std::cout << "Failed to convert from rfl::Generic (TOML): " << err.what() << std::endl;
        return rfl::Result<Eigen::Matrix<double, 2, 3, Eigen::RowMajor>>(err);
    });

    return 0;
}
