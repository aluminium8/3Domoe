#include "rfl_eigen_serdes.hpp" // あなたが作成したヘッダ
#include <iostream>
#include "rfl/json.hpp"
#include "rfl/toml.hpp"

// 簡単なテスト用の構造体
struct MyData
{
    Eigen::Matrix<double, 3, 3, Eigen::RowMajor> mat;
};

int main()
{
    using MatrixType = Eigen::Matrix<double, 2, 3, Eigen::RowMajor>;

    // --- 1. JSONでの成功ケース ---
    std::cout << "--- 1. Successful Deserialization from JSON ---" << std::endl;
    std::string json_ok = R"({
        "storageOrder": "RowMajor",
        "data": [
            [1.0, 2.0, 3.0],
            [4.0, 5.0, 6.0]
        ]
    })";

    auto result1 = rfl::json::read<rfl::Generic>(json_ok)
                       .and_then([](const auto &g)
                                 { return rfl::from_generic<MatrixType>(g); });

    if (result1)
    {
        std::cout << "Successfully converted from rfl::Generic (JSON):\n"
                  << *result1 << std::endl;
    }
    else
    {
        std::cout << "Failed to convert from rfl::Generic (JSON): " << result1.error().what() << std::endl;
    }

    std::cout << std::endl;

    // --- 2. JSONでの失敗ケース (検証ロジック) ---
    std::cout << "--- 2. Failed Deserialization from JSON (Validation Check) ---" << std::endl;
    std::string json_fail = R"({
        "storageOrder": "ColMajor",
        "data": [ [1, 2, 3], [4, 5, 6] ]
    })";

    auto result2 = rfl::json::read<rfl::Generic>(json_fail)
                       .and_then([](const auto &g)
                                 { return rfl::from_generic<MatrixType>(g); });

    if (result2)
    {
        std::cout << "Successfully converted from rfl::Generic (JSON):\n"
                  << *result2 << std::endl;
    }
    else
    {
        std::cout << "Failed to convert from rfl::Generic (JSON): " << result2.error().what() << std::endl;
    }

    std::cout << std::endl;

    //// --- 3. tomlでの成功ケース ---
    //std::cout << "--- 3. Successful Deserialization from toml ---" << std::endl;
    //std::string toml_ok = R"(
    //    storageOrder = "RowMajor"
    //    data = [
    //        [10.0, 20.0, 30.0],
    //        [40.0, 50.0, 60.0]
    //    ]
    //)";
//
    //auto result3 = rfl::toml::read<rfl::Generic>(toml_ok)
    //                   .and_then([](const auto &g)
    //                             { return rfl::from_generic<MatrixType>(g); });
//
    //if (result3)
    //{
    //    std::cout << "Successfully converted from rfl::Generic (toml):\n"
    //              << *result3 << std::endl;
    //}
    //else
    //{
    //    std::cout << "Failed to convert from rfl::Generic (toml): " << result3.error().what() << std::endl;
    //}

    // --- 4. jsonで，構造体情報から一部のみを引き抜く成功ケース ---
    std::cout << "--- 4. Extracting a Matrix from a Struct (json) ---" << std::endl;

    // MyDataインスタンスを作成
    MyData md;
    md.mat << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7, 8, 9;

    // MyDataをjson文字列にシリアライズ
    std::string json_ok_struct = rfl::json::write(md);
    std::cout << "Serialized json from struct:\n"
              << json_ok_struct << std::endl;

    // ▼▼▼ 修正点2: "mat"フィールドを抽出してから変換する ▼▼▼
    auto result4 = rfl::json::read<rfl::Generic>(json_ok_struct)
                       .and_then([](const auto &g)
                                 { return g.to_object(); })
                       .and_then([](const auto &root_obj)
                                 { return root_obj.get("mat"); })
                       .and_then([](const auto &mat_generic)
                                 { return rfl::from_generic<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>>(mat_generic); });

    // ▼▼▼ 修正点3: result4 を使うように修正 ▼▼▼
    if (result4)
    {
        std::cout << "Successfully extracted matrix from generic struct (json):\n"
                  << *result4 << std::endl;
    }
    else
    {
        std::cout << "Failed to extract matrix from generic struct (json): " << result4.error().what() << std::endl;
    }

    // --- 5. jsonから動的サイズの行列(2x4)を読み込む ---
    std::cout << "--- 5. Deserializing a dynamic-size Matrix from json ---" << std::endl;
    std::string json_dynamic = R"(
        storageOrder = "ColMajor"
        data = [
            [1.1, 2.2, 3.3, 4.4],
            [5.5, 6.6, 7.7, 8.8]
        ]
    )";

    auto result5 = rfl::json::read<rfl::Generic>(json_dynamic)
                       .and_then([](const auto &g) { // ★★★ ターゲットの型を Eigen::MatrixXd , Eigen::Dynamic, Eigen::Dynamic ★★★
                           return rfl::from_generic<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>(g);
                       });

    if (result5)
    {
        std::cout << "Successfully converted to dynamic matrix (json):\n"
                  << *result5 << std::endl;
        std::cout << "Size: " << result5->rows() << "x" << result5->cols() << std::endl;
    }
    else
    {
        std::cout << "Failed to convert to dynamic matrix (json): " << result5.error().what() << std::endl;
    }

    return 0;
}