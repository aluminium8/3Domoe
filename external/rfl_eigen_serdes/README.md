# rfl_eigen_serdes
serialization/deserialization header lib with reflect-cpp
This is header only library.

Note: This library is not optimized about performance now. 

Sample

```C++:sample.cpp

#include <string>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <Eigen/Dense>

#include "rfl_eigen_serdes.hpp"

struct ModelColMajor {
    std::string name;
    // Eigen::MatrixXdはデフォルトで列優先 (ColMajor)
    Eigen::MatrixXd coefficients;
};

struct ModelRowMajor {
    std::string name;
    // 明示的に行優先 (RowMajor) の行列を定義
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> coefficients;
};

int main() {
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

    return 0;
}
```