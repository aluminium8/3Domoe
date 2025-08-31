#include <iostream>
#include <string>
#include <vector>

#include "rfl.hpp"
#include "rfl/from_generic.hpp"
#include "rfl/json.hpp"

// 出力用のヘルパー関数
void print_matrix(const std::vector<std::vector<double>>& matrix) {
    std::cout << "Extracted matrix V:" << std::endl;
    for (const auto& row : matrix) {
        for (const auto& val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
    // 元の構造が不明なjsonデータ
    const std::string json_string = R"(
        title = "json Example"
        
        [owner]
        name = "Tom Preston-Werner"
        
        V = [
          [1.1, 2.2, 3.3],
          [4.4, 5.5, 6.6]
        ]

        [database]
        enabled = true
    )";

    // 1. jsonをrfl::Genericにパースする
    const rfl::Result<rfl::Generic> parsed_json =
        rfl::json::read<rfl::Generic>(json_string);

    if (!parsed_json) {
        std::cerr << "Failed to parse json: " << parsed_json.error().what()
                  << std::endl;
        return 1;
    }

    // 2. "owner"テーブル内の"V"フィールドをrfl::Genericとして抽出する (修正箇所)
    const rfl::Result<rfl::Generic> V_generic =
        parsed_json->to_object()
        .and_then([](const rfl::Generic::Object& root_obj) {
            return root_obj.get("owner");
        })
        .and_then([](const rfl::Generic& owner_generic) {
            return owner_generic.to_object();
        })
        .and_then([](const rfl::Generic::Object& owner_obj) {
            return owner_obj.get("V");
        });

    if (!V_generic) {
        std::cerr << "Failed to extract field 'V': " << V_generic.error().what()
                  << std::endl;
        return 1;
    }

    // 3. 抽出したrfl::Genericを目的の型に変換する
    const rfl::Result<std::vector<std::vector<double>>> V_matrix =
        rfl::from_generic<std::vector<std::vector<double>>>(*V_generic);

    if (V_matrix) {
        print_matrix(*V_matrix);
    } else {
        std::cerr << "Failed to convert 'V' to vector<vector<double>>: "
                  << V_matrix.error().what() << std::endl;
        return 1;
    }

    return 0;
}