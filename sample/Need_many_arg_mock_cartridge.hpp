#pragma once

#include "MITSUDomoe/ICartridge.hpp"
#include <rfl.hpp>
#include <vector>
#include <string>
#include "MITSUDomoe/3D_objects.hpp"

class Need_many_arg_mock_cartridge
{
public:
    // 1. 入力用の構造体を定義
    struct Input
    {
        unsigned int input_mesh_id;
        int i;
        long l;
        long long ll;
        double d;
        std::string s;
        MITSU_Domoe::Polygon_mesh pm;
        MITSU_Domoe::Polygon_clusters pc;
        MITSU_Domoe::Point_cloud pointcloud;

    };
    
    std::string command_name="Need_many_arg_mock";
    // 2. 出力用の構造体を定義 (複数の値を返す例)
    struct Output
    {
        unsigned int generated_point_cloud_id;
        size_t num_generated_points;
        std::string message;
    };
    std::string description = "Mock cartridge of polygon processing";
    // 3. 処理本体を実装
    Output execute(const Input &input) const
    {
        std::cout << "\n--- Executing GenerateCentroidsCartridge ---" << std::endl;
        std::cout << "  Processing Mesh ID: " << input.input_mesh_id << std::endl;

        // ... 実際の重心計算処理 ...
        const size_t generated_count = 1234; // 仮の計算結果

        // Output構造体に結果を詰めて返す
        return Output{
            .generated_point_cloud_id = input.input_mesh_id + 1000,
            .num_generated_points = generated_count,
            .message = "Successfully generated " + std::to_string(generated_count) + " points."};
    }
};

// コンセプトを満たしていることをコンパイラにチェックさせる (任意)
static_assert(MITSU_Domoe::Cartridge<Need_many_arg_mock_cartridge>);
