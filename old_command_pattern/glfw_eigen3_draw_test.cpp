#include <iostream>
#include <vector>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Eigen/Core>

// --- デバッグ用: OpenGLエラーチェック関数 ---
// OpenGLのエラーをチェックし、コンソールに出力する
GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    // ループですべてのエラーを取得する
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            error = "STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error = "STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cerr << "OpenGL Error: " << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
// マクロにして、ファイル名と行番号を自動で渡す
#define glCheckError() glCheckError_(__FILE__, __LINE__)

// --- デバッグ用: GLFWエラーコールバック関数 ---
void error_callback(int error, const char *description)
{
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

// --- シェーダーソース (変更なし) ---
const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec4 aPos;
void main() { gl_Position = vec4(aPos); }
)glsl";

const char *fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); }
)glsl";

// --- シェーダーコンパイル用ヘルパー関数 (変更なし) ---
GLuint createShaderProgram()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        throw std::runtime_error("SHADER::VERTEX::COMPILATION_FAILED\n" + std::string(infoLog));
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        throw std::runtime_error("SHADER::FRAGMENT::COMPILATION_FAILED\n" + std::string(infoLog));
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        throw std::runtime_error("SHADER::PROGRAM::LINKING_FAILED\n" + std::string(infoLog));
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// --- メイン処理 ---
int main()
{
    // 追加点1: GLFWのエラーコールバックを設定
    glfwSetErrorCallback(error_callback);

    // 追加点2: main関数全体をtry-catchで囲む
    try
    {
        if (!glfwInit())
        {
            // エラーコールバックが呼ばれるが、念のため例外をスロー
            throw std::runtime_error("Failed to initialize GLFW");
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(800, 600, "Eigen Polygon Drawing (Double)", NULL, NULL);
        if (window == NULL)
        {
            throw std::runtime_error("Failed to create GLFW window");
        }
        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        // 最初のOpenGLエラーチェック
        glCheckError();

        using PolygonMatrix = Eigen::Matrix<double, 3, 4, Eigen::RowMajor>;
        struct matrix_vector
        {
            std::vector<PolygonMatrix> matv;
            
        };

        matrix_vector polygons;
        polygons.matv.resize(2);
        polygons.matv[0] << -0.9, -0.9, 0.0, 1.0, -0.1, -0.9, 0.0, 1.0, -0.5, -0.1, 0.0, 1.0;
        polygons.matv[1] << 0.1, 0.1, 0.0, 1.0, 0.9, 0.1, 0.0, 1.0, 0.5, 0.9, 0.0, 1.0;

        GLuint shaderProgram = createShaderProgram();
        glCheckError(); // シェーダー作成後のチェック

        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, polygons.matv.size() * sizeof(PolygonMatrix), polygons.matv.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_DOUBLE, GL_FALSE, 4 * sizeof(double), (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glCheckError(); // VAO/VBO設定後のチェック

        while (!glfwWindowShouldClose(window))
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, polygons.matv.size() * 3);

            // 追加点3: 描画ループの最後にエラーがないかチェック
            glCheckError();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
    catch (const std::exception &e)
    {
        // 発生した例外を標準エラー出力に書き出す
        std::cerr << "Exception caught: " << e.what() << std::endl;
        glfwTerminate(); // 先にterminateしないとウィンドウが残ることがある
        return -1;
    }

    glfwTerminate();
    return 0;
}