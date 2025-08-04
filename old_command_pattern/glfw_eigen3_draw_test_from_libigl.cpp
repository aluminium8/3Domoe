// このファイル全体をコピーして、あなたの main.cpp に上書きしてください。

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <numbers>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <igl/readSTL.h>

// --- データ構造 ---
using PolygonMatrix = Eigen::Matrix<double, 3, 4, Eigen::RowMajor>;
struct matrix_vector {
    std::vector<PolygonMatrix> matv;
};

// --- 関数プロトタイプ ---
matrix_vector readSTLToMatrixVector(const std::string& filename);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
Eigen::Matrix4f createPerspectiveMatrix(float fov, float aspect, float near_plane, float far_plane);

// --- シェーダー ---
const char *vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

const char *fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(0.7, 0.7, 0.7, 1.0);
    }
)glsl";


int main(int argc, char *argv[])
{
    // --- 1. STLファイルの読み込み ---
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_your_stl_file.stl>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    matrix_vector data = readSTLToMatrixVector(filename);
    if (data.matv.empty()) {
        std::cerr << "Could not load model, exiting." << std::endl;
        return -1;
    }
    std::cout << "\n--- Starting Graphics Window ---" << std::endl;

    // --- 2. GLFWとGLADの初期化 ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "STL Viewer (Eigen Version)", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // --- 3. シェーダーのコンパイルとリンク ---
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- 4. 頂点データの準備とGPUへのアップロード ---
    std::vector<float> vertices;
    for (const auto& poly : data.matv) {
        for (int i = 0; i < 3; ++i) {
            vertices.push_back(static_cast<float>(poly(i, 0)));
            vertices.push_back(static_cast<float>(poly(i, 1)));
            vertices.push_back(static_cast<float>(poly(i, 2)));
        }
    }

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // --- 5. 描画ループ ---
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
        Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
        Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

        float angle = (float)glfwGetTime() * 0.3f;
        Eigen::AngleAxisf rotation(angle, Eigen::Vector3f(0.5f, 1.0f, 0.0f).normalized());
        
        // ★★★★★★★★★★★★★★★★★★★ 修正点 ★★★★★★★★★★★★★★★★★★★
        // 4x4行列の左上3x3部分に、3x3の回転行列を代入する
        model.block<3,3>(0,0) = rotation.toRotationMatrix();
        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★

        Eigen::Affine3f view_transform = Eigen::Affine3f::Identity();
        view_transform.translate(Eigen::Vector3f(0.0f, 0.0f, -3.0f));
        view = view_transform.matrix();

        projection = createPerspectiveMatrix(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model.data());
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.data());
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection.data());

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 3));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- 6. リソースの解放 ---
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}


// --- ヘルパー関数とコールバック関数の実装 ---

Eigen::Matrix4f createPerspectiveMatrix(float fov_deg, float aspect, float near_plane, float far_plane) {
    Eigen::Matrix4f p = Eigen::Matrix4f::Zero();
    float fov_rad = fov_deg * static_cast<float>(std::numbers::pi) / 180.0f;
    float tanHalfFovy = std::tan(fov_rad / 2.0f);

    p(0, 0) = 1.0f / (aspect * tanHalfFovy);
    p(1, 1) = 1.0f / (tanHalfFovy);
    p(2, 2) = -(far_plane + near_plane) / (far_plane - near_plane);
    p(3, 2) = -1.0f;
    p(2, 3) = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);

    return p;
}

matrix_vector readSTLToMatrixVector(const std::string& filename) {
    matrix_vector result;
    
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    Eigen::MatrixXd N;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return result;
    }
    
    if (!igl::readSTL(file, V, F, N)) {
        std::cerr << "Error: Failed to read STL data from stream: " << filename << std::endl;
        return result;
    }
    
    for (int i = 0; i < F.rows(); ++i) {
        PolygonMatrix triangle_matrix;
        const Eigen::RowVector3d p1 = V.row(F(i, 0));
        const Eigen::RowVector3d p2 = V.row(F(i, 1));
        const Eigen::RowVector3d p3 = V.row(F(i, 2));
        triangle_matrix.row(0) << p1, 1.0;
        triangle_matrix.row(1) << p2, 1.0;
        triangle_matrix.row(2) << p3, 1.0;
        result.matv.push_back(triangle_matrix);
    }
    
    std::cout << "Successfully loaded " << result.matv.size() << " triangles from " << filename << std::endl;
    if(!result.matv.empty()){
        std::cout << "\nMatrix for the first triangle:" << std::endl;
        std::cout << result.matv[0] << std::endl;
    }
    
    return result;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}