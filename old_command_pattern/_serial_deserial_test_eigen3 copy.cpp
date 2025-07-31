#include <string>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <rfl/toml.hpp>
#include <Eigen/Dense>

#include "rfl_eigen_serdes.hpp"

struct MESH_ID
{
    uint64_t id;
    uint64_t id2;
};

struct PublicInput
{
    MESH_ID source_mesh_id;
};
struct ModelColMajor
{
    std::string name;
    // Eigen::MatrixXdはデフォルトで列優先 (ColMajor)
    Eigen::MatrixXd coefficients;
};

struct ModelRowMajor
{
    std::string name;
    // 明示的に行優先 (RowMajor) の行列を定義
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> coefficients;
};

struct MESH2_ID
{
    int id;
};

struct MESH2
{
    std::string s;
};

struct test_args
{
    MESH2_ID mesh_id;
    std::string msg;
};

struct test_args_translated
{
    MESH2 mesh;
    std::string msg;
};

int main()
{
    {
        // ------------------- シリアライズ (RowMajor) -------------------
        ModelRowMajor model_row_major;
        model_row_major.name = "RowMajor ModelColMajor";
        model_row_major.coefficients.resize(2, 2);
        model_row_major.coefficients << 1, 2, 3, 4;
        auto json_row_major = rfl::json::write(model_row_major);
        std::cout << "Serialized RowMajor JSON: " << json_row_major << std::endl;

        // ------------------- デシリアライズのテスト -------------------
        std::cout << "\n--- Test Case 1: Matching types (SUCCESS) ---" << std::endl;
        // RowMajorのJSONをRowMajorの型にデシリアライズ -> 成功する
        auto res1 = rfl::json::read<ModelRowMajor>(json_row_major);
        assert(res1.has_value());
        std::cout << "Successfully deserialized RowMajor JSON into RowMajor type." << std::endl;
        assert(res1.value().coefficients.IsRowMajor);

        std::cout << "\n--- Test Case 2: Mismatched types (estimated to be failed) ---" << std::endl;
        // RowMajorのJSONをColMajorの型にデシリアライズ -> 失敗する
        auto res2 = rfl::json::read<ModelColMajor>(json_row_major);
        assert(!res2.has_value()); // 失敗を表明
        std::cout << "Correctly failed to deserialize RowMajor JSON into ColMajor type." << std::endl;
        std::cout << "Error message: " << res2.error().what() << std::endl;
    }
    std::cout << "\n--- Test Case 3: nested struct ---" << std::endl;
    {
        PublicInput pi;
        pi.source_mesh_id.id = 1;
        pi.source_mesh_id.id2 = 1;
        std::string ser_pi = rfl::toml::write(pi);
        std::cout << ser_pi << std::endl;
        PublicInput pi_des = rfl::toml::read<PublicInput>(ser_pi).value();
        std::cout << pi_des.source_mesh_id.id << " " << pi_des.source_mesh_id.id2 << std::endl;
    }

    std::cout << "\n--- Test Case 4: translate ---" << std::endl;

    test_args t;
    t.mesh_id.id = 1;
    t.msg = "aiueo";
    test_args_translated tt;
    auto t_view=rfl::to_view(t);
    auto tt_view=rfl::to_view(tt);
    
    for(const auto& f : rfl::fields<test_args>()){
        std::cout<<f.name()<<" "<<f.type()<<std::endl;
        *tt_view.get<f.name()>()=*t_view.get<f.name()>();
    }

    return 0;
}