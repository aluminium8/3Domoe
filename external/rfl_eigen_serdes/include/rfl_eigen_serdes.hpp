#pragma once

#include <rfl.hpp>

#include <Eigen/Dense>

#include <stdexcept>
#include <string>
#include <vector>

// シリアライズ用の中間構造体（変更なし）
template <typename T>
struct SerializableEigenMatrix
{
    std::string storageOrder; // "RowMajor" または "ColMajor"
    std::vector<std::vector<T>> data;
};

namespace rfl
{
    // Eigen::MatrixのReflectorを修正
    template <typename T, int R, int C, int O, int MR, int MC>
    struct Reflector<Eigen::Matrix<T, R, C, O, MR, MC>>
    {
        using Matrix = Eigen::Matrix<T, R, C, O, MR, MC>;
        using ReflType = SerializableEigenMatrix<T>;

        // fromメソッドは変更なし
        static ReflType from(const Matrix &m)
        {
            ReflType s;
            s.storageOrder = (m.IsRowMajor) ? "RowMajor" : "ColMajor";

            s.data.resize(m.rows(), std::vector<T>(m.cols()));
            for (int r = 0; r < m.rows(); ++r)
            {
                for (int c = 0; c < m.cols(); ++c)
                {
                    s.data[r][c] = m(r, c);
                }
            }
            return s;
        }

        // toメソッドに検証ロジックを追加
        static Matrix to(const ReflType &s)
        {
            // ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼
            // ▼▼▼ 追加した検証ロジック ▼▼▼
            // ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼

            // デシリアライズ先のC++型が期待するストレージ順序 (コンパイル時に決定)
            constexpr bool target_is_row_major = Matrix::IsRowMajor;

            // JSONデータが示すストレージ順序 (実行時に決定)
            const bool source_is_row_major = (s.storageOrder == "RowMajor"); //
            const bool source_is_col_major = (s.storageOrder == "ColMajor");
            if (!(source_is_row_major || source_is_col_major))
            {
                std::string msg = "Storage order tag is not valid: JSON data is '" + s.storageOrder +
                                  "' but the expects are '" + "RowMajor / ColMajor" + "'.";
                // 例外を投げるとrfl::readがキャッチしてrfl::Result<Error>にしてくれる
                throw std::runtime_error(msg);
            }
            if (target_is_row_major != source_is_row_major)
            {
                std::string expected = target_is_row_major ? "RowMajor" : "ColMajor";
                std::string msg = "Storage order mismatch: JSON data is '" + s.storageOrder +
                                  "' but the C++ type expects '" + expected + "'.";
                // 例外を投げるとrfl::readがキャッチしてrfl::Result<Error>にしてくれる
                throw std::runtime_error(msg);
            }

            // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲
            // ▲▲▲ ここまで ▲▲▲
            // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲

            if (s.data.empty() || s.data[0].empty())
            {
                return Matrix(0, 0);
            }

            const auto rows = s.data.size();
            const auto cols = s.data[0].size();

            Matrix m(rows, cols);
            for (size_t r = 0; r < rows; ++r)
            {
                for (size_t c = 0; c < cols; ++c)
                {
                    m(r, c) = s.data[r][c];
                }
            }
            return m;
        }
    };

} // namespace rfl