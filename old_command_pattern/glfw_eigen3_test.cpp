
#include <iostream>
#include <vector>
#include <regex>
#include "rfl.hpp"
#include "rfl/json.hpp"
// Accuracy: can accurately read and write int64, uint64, and double numbers. [https://github.com/ibireme/yyjson]
//->
//"vertice":[{"position":{"storageOrder":"ColMajor","data":[[0.5],[0.5],[0.0]]},"color":{"storageOrder":"ColMajor","data":[[1.0],[0.0],[0.0]]},
//    "s":"aiueo","b":true,"i":1152921504606846979,"d":1.000000001},...
#include "rfl/toml.hpp"
#include <Eigen/Dense>
#include <string>
// 64bit (rule)
//->
//[[vertice]]
// b = true
// d = 1.000000001
// i = 1152921504606846979
// s = 'aiueo'
//
//     [vertice.color]
//     data = [ [ 1.0 ], [ 0.0 ], [ 0.0 ] ]
//     storageOrder = 'ColMajor'
//
//     [vertice.position]
//     data = [ [ 0.5 ], [ 0.5 ], [ 0.0 ] ]
//     storageOrder = 'ColMajor'
//[[vertice]]...
#include "rfl/yaml.hpp"
// big int (rule)
//->
// vertice:
//   - position:
//       storageOrder: ColMajor
//       data:
//         -
//           - 0.500000
//         -
//           - 0.500000
//         -
//           - 0.000000
//     color:
//       storageOrder: ColMajor
//       data:
//         -
//           - 1.000000
//         -
//           - 0.000000
//         -
//           - 0.000000
//     s: aiueo
//     b: true
//     i: 1152921504606846979
//     d: 1.000000
//   - position: ...
////yaml maybe not have enough precision
#include "rfl_eigen_serdes.hpp"
#include "string"

// GLAD (OpenGL拡張をロード)
// ※ glad.c もコンパイル対象に含めること
#include <glad/glad.h>

// GLFW (ウィンドウと入力を管理)
#include <GLFW/glfw3.h>

// Eigen (線形代数ライブラリ)
#include <Eigen/Core>

// --- シェーダーソース ---
// 頂点シェーダー
const char *vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    out vec3 ourColor;

    void main()
    {
        gl_Position = vec4(aPos, 1.0);
        ourColor = aColor;
    }
)glsl";

// フラグメントシェーダー
const char *fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec3 ourColor;

    void main()
    {
        FragColor = vec4(ourColor, 1.0f);
    }
)glsl";

// 頂点データを格納する構造体
struct Vertex
{
    Eigen::Vector3f position;
    Eigen::Vector3f color;
    std::string s = "aiueo";
    bool b = true;
    long long int i = 1152921504606846979;
    double d = 1 + 1e-09;
};

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

// ウィンドウ設定
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

struct Polygon_mesh
{
    std::vector<Vertex> vertice;
};

// ヘルパー関数1: 文字列が数値配列の「中身」として妥当かチェックする関数
bool is_numeric_array_content(const std::string& content) {
    if (std::regex_match(content, std::regex(R"(^\s*$)"))) {
        return false;
    }
    // ★★★ 修正点 ★★★
    // 許可する文字に '[' と ']' を追加。これにより、ネストした配列も対象になる。
    // 元: ^[\s\d.,-]*$
    return std::regex_match(content, std::regex(R"(^[\s\d.,\[\]-]*$)"));
}

// ヘルパー関数2: 配列の中身の空白やコンマを整形する
std::string format_array_content(std::string content) {
    content = std::regex_replace(content, std::regex(R"(\s+)"), " ");
    content = std::regex_replace(content, std::regex(R"(\s+,)"), ",");
    content = std::regex_replace(content, std::regex(R"(,(?!\s))"), ", ");
    content = std::regex_replace(content, std::regex(R"(^\s+|\s+$)"), "");
    return content;
}

/**
 * @brief 文字列を受け取り、その中の数値配列のみを1行に整形して返す
 * @param original_str 整形したい文字列
 * @return std::string 整形後の文字列
 */

// 配列の中身（数値とコンマ）の空白を整形するヘルパー関数
std::string compact_numbers(std::string content) {
    content = std::regex_replace(content, std::regex(R"(\s+)"), " ");
    content = std::regex_replace(content, std::regex(R"(\s*,\s*)"), ", ");
    content = std::regex_replace(content, std::regex(R"(^\s+|\s+$)"), "");
    return content;
}

// 見つけ出した配列ブロックを美しく整形する関数
// ★★★ base_indent を引数に追加 ★★★
std::string pretty_print_array(const std::string& block, const std::string& base_indent) {
    const std::string single_indent = "    "; // 1階層分のインデント
    std::string content = block.substr(1, block.length() - 2);

    if (content.find('[') != std::string::npos) {
        // --- ネストした配列の整形ロジック ---
        std::regex inner_finder(R"(\[([\s\S]*?)\])");
        std::smatch inner_match;
        auto search_start = content.cbegin();
        
        std::vector<std::string> formatted_inner_arrays;
        while(std::regex_search(search_start, content.cend(), inner_match, inner_finder)) {
            formatted_inner_arrays.push_back(compact_numbers(inner_match[1].str()));
            search_start = inner_match.suffix().first;
        }

        if (formatted_inner_arrays.empty()) return block;

        std::stringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < formatted_inner_arrays.size(); ++i) {
            // ベースのインデントに、さらにインデントを追加
            ss << base_indent << single_indent << "[" << formatted_inner_arrays[i] << "]";
            if (i < formatted_inner_arrays.size() - 1) {
                ss << ",\n";
            } else {
                ss << "\n";
            }
        }
        // ベースのインデントに合わせて閉じる
        ss << base_indent << "]";
        return ss.str();

    } else {
        // --- ネストしていない単純な配列の整形ロジック ---
        return "[" + compact_numbers(content) + "]";
    }
}

// メインの整形関数（パーサー方式）
std::string format_string(const std::string& original_str) {
    std::string result = original_str;
    size_t start_pos = 0;
    const std::string target_key = R"("data":)";

    while ((start_pos = result.find(target_key, start_pos)) != std::string::npos) {
        // ★★★ インデント検出ロジックを追加 ★★★
        size_t line_start = result.rfind('\n', start_pos);
        line_start = (line_start == std::string::npos) ? 0 : line_start + 1;
        size_t first_char_pos = result.find_first_not_of(" \t", line_start);
        const std::string base_indent = result.substr(line_start, first_char_pos - line_start);
        
        size_t array_start = result.find('[', start_pos + target_key.length());
        if (array_start == std::string::npos) continue;

        int depth = 0;
        size_t array_end = std::string::npos;
        for (size_t i = array_start; i < result.length(); ++i) {
            if (result[i] == '[') depth++;
            else if (result[i] == ']') {
                if (--depth == 0) {
                    array_end = i;
                    break;
                }
            }
        }
        if (array_end == std::string::npos) continue;

        size_t block_len = array_end - array_start + 1;
        std::string block_to_format = result.substr(array_start, block_len);

        // ★★★ 整形関数にインデントを渡す ★★★
        std::string formatted_block = pretty_print_array(block_to_format, base_indent);
        
        result.replace(array_start, block_len, formatted_block);
        
        start_pos = array_start + formatted_block.length();
    }
    
    return result;
}


struct matrix_vector
{
    std::vector<Eigen::Matrix<double, 3, 4>> matv;
};

int main()
{

    // --- GLFWの初期化 ---
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // --- ウィンドウの作成 ---
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "DrawArrays Full Example", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // --- GLADの初期化 ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // --- シェーダーのコンパイルとリンク ---
    // 頂点シェーダー
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // コンパイルエラーのチェック
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // フラグメントシェーダー
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // コンパイルエラーのチェック
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // シェーダープログラム
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // リンクエラーのチェック
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- ポリゴンデータの定義 (indices なし) ---
    // 2つの三角形を構成するために、6つの頂点を定義する
    std::vector<Vertex> vertices = {
        // 1つ目の三角形
        {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 右上 (赤)
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // 右下 (緑)
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}}, // 左上 (黄)
        // 2つ目の三角形
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // 右下 (緑)
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // 左下 (青)
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}}   // 左上 (黄)
    };

    // --- VAO, VBO のセットアップ (EBO は不要) ---
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // 頂点属性ポインタの設定
    // 位置属性 (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // 色属性 (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // バインド解除 (任意)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- 描画ループ ---
    while (!glfwWindowShouldClose(window))
    {
        // 入力処理
        processInput(window);

        // レンダリングコマンド
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ポリゴンの描画
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // 描画命令を glDrawArrays に変更
        // 第2引数は開始インデックス、第3引数は頂点の数
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        // バッファのスワップとイベントのポーリング
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- リソースの解放 ---
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();

    Polygon_mesh pm;
    pm.vertice = vertices;

    auto v_json = rfl::json::write(pm, rfl::json::pretty);
    std::cout << v_json << std::endl;
    std::string v_json_compact = format_string(v_json);
    std::cout << v_json_compact << std::endl;
    Polygon_mesh pm2_json = rfl::json::read<Polygon_mesh>(v_json).value();
    std::cout << pm2_json.vertice[0].position.transpose() << "\n"
              << pm2_json.vertice[1].position.transpose() << "\n"
              << pm2_json.vertice[2].position.transpose() << "\n"
              << pm2_json.vertice[3].position.transpose() << "\n"
              << pm2_json.vertice[4].position.transpose() << "\n"
              << pm2_json.vertice[5].position.transpose() << std::endl;

    auto v_toml = rfl::toml::write(pm);
    std::cout << v_toml << std::endl;
    Polygon_mesh pm2_toml = rfl::toml::read<Polygon_mesh>(v_toml).value();
    std::cout << pm2_toml.vertice[0].position.transpose() << "\n"
              << pm2_toml.vertice[1].position.transpose() << "\n"
              << pm2_toml.vertice[2].position.transpose() << "\n"
              << pm2_toml.vertice[3].position.transpose() << "\n"
              << pm2_toml.vertice[4].position.transpose() << "\n"
              << pm2_toml.vertice[5].position.transpose() << std::endl;

    auto v_yaml = rfl::yaml::write(pm);
    std::cout << v_yaml << std::endl;
    Polygon_mesh pm2_yaml = rfl::yaml::read<Polygon_mesh>(v_yaml).value();
    std::cout << pm2_yaml.vertice[0].position.transpose() << "\n"
              << pm2_yaml.vertice[1].position.transpose() << "\n"
              << pm2_yaml.vertice[2].position.transpose() << "\n"
              << pm2_yaml.vertice[3].position.transpose() << "\n"
              << pm2_yaml.vertice[4].position.transpose() << "\n"
              << pm2_yaml.vertice[5].position.transpose() << std::endl;

    // 構造体のインスタンスを作成
    matrix_vector mv;

    // 格納したい行列の数
    const int num_matrices_to_add = 5;

    // forループを使って、指定した数の行列を生成し、vectorに追加する
    for (int i = 0; i < num_matrices_to_add; ++i)
    {
        // 3x4の行列を生成
        Eigen::Matrix<double, 3, 4> temp_matrix;

        // --- 値の代入 ---
        // 方法1: ランダムな値で初期化
        temp_matrix.setRandom();

        // 方法2: 特定の規則に基づいた値で初期化 (例: i, 行, 列に基づく値)
        /*
        for (int row = 0; row < temp_matrix.rows(); ++row) {
            for (int col = 0; col < temp_matrix.cols(); ++col) {
                temp_matrix(row, col) = 100 * i + 10 * row + col;
            }
        }
        */

        // 生成した行列をvectorの末尾に追加
        mv.matv.push_back(temp_matrix);
    }
    mv.matv[0](0,0)=1+1e-12;

    auto matv_json = rfl::json::write(mv, rfl::json::pretty);

    std::cout << matv_json << std::endl;
    std::string matv_json_compact = format_string(matv_json);
    std::cout << matv_json_compact << std::endl;
    auto mv2_json = rfl::json::read<matrix_vector>(matv_json).value();
        // forループを使って、指定した数の行列を生成し、vectorに追加する
    for (int i = 0; i < num_matrices_to_add; ++i)
    {
        std::cout<<mv2_json.matv[i]<<" /"<<std::endl;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// ヘルパー関数
// -----------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}