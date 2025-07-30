// 1. main関数を自動生成するためにMODULEを定義
#define BOOST_TEST_MODULE EigenSerializationTests

#include <boost/test/included/unit_test.hpp>
#include "rfl_eigen_serdes.hpp" // テスト対象のヘッダ
#include <Eigen/Dense>
#include <string>

// ------------------- テスト用の構造体定義 -------------------

// デフォルトの列優先 (ColMajor) 行列を持つ構造体
struct ModelColMajor
{
    std::string name;
    Eigen::MatrixXd coefficients;
};

// 行優先 (RowMajor) 行列を持つ構造体
struct ModelRowMajor
{
    std::string name;
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> coefficients;
};

// 固定サイズの行列を持つ構造体
struct ModelFixedSize
{
    std::string name;
    Eigen::Matrix3f coefficients;
};

// 整数型の行列を持つ構造体
struct ModelInt
{
    std::string name;
    Eigen::Matrix<int, 2, 2> coefficients;
};

// ------------------- ヘルパー関数 -------------------

// 浮動小数点数を含む行列を比較するためのヘルパー関数
template <typename T1, typename T2>
void check_matrix_close(const T1 &m1, const T2 &m2, double tolerance = 1e-9)
{
    BOOST_REQUIRE_EQUAL(m1.rows(), m2.rows());
    BOOST_REQUIRE_EQUAL(m1.cols(), m2.cols());
    for (int r = 0; r < m1.rows(); ++r)
    {
        for (int c = 0; c < m1.cols(); ++c)
        {
            BOOST_CHECK_CLOSE(static_cast<double>(m1(r, c)), static_cast<double>(m2(r, c)), tolerance);
        }
    }
}

// 整数を含む行列を比較するためのヘルパー関数
template <typename T1, typename T2>
void check_matrix_equal(const T1 &m1, const T2 &m2)
{
    BOOST_REQUIRE_EQUAL(m1.rows(), m2.rows());
    BOOST_REQUIRE_EQUAL(m1.cols(), m2.cols());
    for (int r = 0; r < m1.rows(); ++r)
    {
        for (int c = 0; c < m1.cols(); ++c)
        {
            BOOST_CHECK_EQUAL(m1(r, c), m2(r, c));
        }
    }
}

// ------------------- テストスイート -------------------

BOOST_AUTO_TEST_SUITE(EigenSerializationTestSuite)

// --- 正常系のテスト ---

BOOST_AUTO_TEST_CASE(row_major_to_row_major_succeeds)
{
    ModelRowMajor original;
    original.name = "Test Row Major";
    original.coefficients.resize(2, 3);
    original.coefficients << 1.1, 2.2, 3.3, 4.4, 5.5, 6.6;

    const std::string json_str = rfl::json::write(original);
    const auto result = rfl::json::read<ModelRowMajor>(json_str);
    std::cout<<result.value().coefficients<<std::endl;
    
    std::cout<<result.has_value()<<std::endl;
    BOOST_REQUIRE(result.has_value());
    const auto &deserialized = result.value();
    BOOST_CHECK_EQUAL(original.name, deserialized.name);
    BOOST_CHECK(deserialized.coefficients.IsRowMajor);
    check_matrix_close(original.coefficients, deserialized.coefficients);
}

BOOST_AUTO_TEST_CASE(col_major_to_col_major_succeeds)
{
    ModelColMajor original;
    original.name = "Test Col Major";
    original.coefficients.resize(3, 2);
    original.coefficients << 1.1, 2.2, 3.3, 4.4, 5.5, 6.6;

    const std::string json_str = rfl::json::write(original);
    const auto result = rfl::json::read<ModelColMajor>(json_str);

    BOOST_REQUIRE(result.has_value());
    const auto &deserialized = result.value();
    BOOST_CHECK_EQUAL(original.name, deserialized.name);
    BOOST_CHECK(deserialized.coefficients.IsRowMajor == false);
    check_matrix_close(original.coefficients, deserialized.coefficients);
}

BOOST_AUTO_TEST_CASE(fixed_size_matrix_succeeds)
{
    ModelFixedSize original;
    original.name = "Fixed Size";
    original.coefficients << 1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f;

    const std::string json_str = rfl::json::write(original);
    const auto result = rfl::json::read<ModelFixedSize>(json_str);

    BOOST_REQUIRE(result.has_value());
    check_matrix_close(original.coefficients, result.value().coefficients);
}

BOOST_AUTO_TEST_CASE(integer_matrix_succeeds)
{
    ModelInt original;
    original.name = "Integer Matrix";
    original.coefficients << -1, 0, 1, 100;

    const std::string json_str = rfl::json::write(original);
    const auto result = rfl::json::read<ModelInt>(json_str);

    BOOST_REQUIRE(result.has_value());
    check_matrix_equal(original.coefficients, result.value().coefficients);
}

BOOST_AUTO_TEST_CASE(empty_matrix_succeeds)
{
    ModelColMajor original;
    original.name = "Empty Matrix";
    original.coefficients.resize(0, 0);

    const std::string json_str = rfl::json::write(original);
    const auto result = rfl::json::read<ModelColMajor>(json_str);

    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result.value().coefficients.rows(), 0);
    BOOST_CHECK_EQUAL(result.value().coefficients.cols(), 0);
}

// --- 異常系のテスト ---

BOOST_AUTO_TEST_CASE(row_major_to_col_major_fails)
{
    ModelRowMajor original_row;
    original_row.name = "Row Major Data";
    original_row.coefficients.resize(2, 2);
    original_row.coefficients << 1, 2, 3, 4;

    const std::string json_str = rfl::json::write(original_row);
    const auto result = rfl::json::read<ModelColMajor>(json_str);

    BOOST_CHECK(!result.has_value());
    //BOOST_REQUIRE(result.error().what());
    const std::string error_msg = result.error().what();
    BOOST_CHECK(error_msg.find("Storage order mismatch") != std::string::npos);
    BOOST_CHECK(error_msg.find("'RowMajor'") != std::string::npos);
    BOOST_CHECK(error_msg.find("'ColMajor'") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(col_major_to_row_major_fails)
{
    ModelColMajor original_col;
    original_col.name = "Col Major Data";
    original_col.coefficients.resize(2, 2);
    original_col.coefficients << 1, 2, 3, 4;

    const std::string json_str = rfl::json::write(original_col);
    const auto result = rfl::json::read<ModelRowMajor>(json_str);

    BOOST_CHECK(!result.has_value());
    //BOOST_REQUIRE(result.error().what());
    const std::string error_msg = result.error().what();
    BOOST_CHECK(error_msg.find("Storage order mismatch") != std::string::npos);
    BOOST_CHECK(error_msg.find("'ColMajor'") != std::string::npos);
    BOOST_CHECK(error_msg.find("'RowMajor'") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(invalid_json_no_storage_order_fails)
{
    const std::string json_str = R"({"name":"Test","coefficients":{"data":[[1,2],[3,4]]}})";

    const auto result = rfl::json::read<ModelColMajor>(json_str);

    BOOST_CHECK(!result.has_value());

    const std::string error_msg = result.error().what();
    //std::cout<<error_msg<<std::endl;
    // storageOrderの領域がないため，配置ができずエラー(Note: デフォルト設定をおいてもいいかもしれない)
    BOOST_CHECK(error_msg.find("Field named 'storageOrder' not found") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(invalid_json_bad_storage_order_value_fails)
{
    const std::string json_str = R"({"name":"Test","coefficients":{"storageOrder":"InvalidValue","data":[[1,2],[3,4]]}})";

    const auto result = rfl::json::read<ModelColMajor>(json_str);
    std::cout<<result.error().what()<<std::endl;
    BOOST_CHECK(!result.has_value());
    //BOOST_REQUIRE(result.error().what());
    const std::string error_msg = result.error().what();
    BOOST_CHECK(error_msg.find("Storage order tag is not valid") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()