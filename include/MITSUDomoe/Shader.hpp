#pragma once

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <filesystem>

#include <Eigen/Dense>

namespace MITSU_Domoe
{

    class Shader
    {
    public:
        unsigned int ID;

        Shader() : ID(0) {}

        // compiles the shader from given source code
        void compile(const char *vertexSource, const char *fragmentSource, const char *geometrySource = nullptr)
        {
            unsigned int sVertex, sFragment, gShader;

            // vertex Shader
            sVertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(sVertex, 1, &vertexSource, NULL);
            glCompileShader(sVertex);
            checkCompileErrors(sVertex, "VERTEX");

            // fragment Shader
            sFragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sFragment, 1, &fragmentSource, NULL);
            glCompileShader(sFragment);
            checkCompileErrors(sFragment, "FRAGMENT");

            // if geometry shader source is provided, also compile geometry shader
            if (geometrySource != nullptr)
            {
                gShader = glCreateShader(GL_GEOMETRY_SHADER);
                glShaderSource(gShader, 1, &geometrySource, NULL);
                glCompileShader(gShader);
                checkCompileErrors(gShader, "GEOMETRY");
            }

            // shader Program
            this->ID = glCreateProgram();
            glAttachShader(this->ID, sVertex);
            glAttachShader(this->ID, sFragment);
            if (geometrySource != nullptr)
                glAttachShader(this->ID, gShader);
            glLinkProgram(this->ID);
            checkCompileErrors(this->ID, "PROGRAM");

            // delete the shaders as they're linked into our program now and no longer necessary
            glDeleteShader(sVertex);
            glDeleteShader(sFragment);
            if (geometrySource != nullptr)
                glDeleteShader(gShader);
        }

        // activate the shader
        void use() const
        {
            glUseProgram(this->ID);
        }

        // utility uniform functions
        void setBool(const std::string &name, bool value) const
        {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
        }
        void setInt(const std::string &name, int value) const
        {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
        }
        void setFloat(const std::string &name, float value) const
        {
            glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
        }
        void setMatrix4fv(const std::string &name, const Eigen::Matrix4f &mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, mat.data());
        }

    private:
        // utility function for checking shader compilation/linking errors.
        void checkCompileErrors(unsigned int shader, std::string type)
        {
            int success;
            char infoLog[1024];
            if (type != "PROGRAM")
            {
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                              << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                }
            }
            else
            {
                glGetProgramiv(shader, GL_LINK_STATUS, &success);
                if (!success)
                {
                    glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                              << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                }
            }
        }
    };

    class ShaderManager
    {
    public:
        std::map<std::string, Shader> Shaders;

        // loads a shader from file and stores it
        Shader loadShader(const std::string &name, const std::string &vShaderFile, const std::string &fShaderFile, const std::string &gShaderFile = "")
        {
            std::string vertexCode;
            std::string fragmentCode;
            std::string geometryCode;
            try
            {
                // open files
                std::ifstream vertexShaderFile(vShaderFile);
                std::ifstream fragmentShaderFile(fShaderFile);
                std::stringstream vShaderStream, fShaderStream;
                // read file's buffer contents into streams
                vShaderStream << vertexShaderFile.rdbuf();
                fShaderStream << fragmentShaderFile.rdbuf();
                // close file handlers
                vertexShaderFile.close();
                fragmentShaderFile.close();
                // convert stream into string
                vertexCode = vShaderStream.str();
                fragmentCode = fShaderStream.str();
                // if geometry shader path is present, also load a geometry shader
                if (gShaderFile != "")
                {
                    std::ifstream geometryShaderFile(gShaderFile);
                    std::stringstream gShaderStream;
                    gShaderStream << geometryShaderFile.rdbuf();
                    geometryShaderFile.close();
                    geometryCode = gShaderStream.str();
                }
            }
            catch (std::exception e)
            {
                std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
            }
            const char *vShaderCode = vertexCode.c_str();
            const char *fShaderCode = fragmentCode.c_str();
            const char *gShaderCode = geometryCode.empty() ? nullptr : geometryCode.c_str();
            // 2. now create shader object from source code
            Shader shader;
            shader.compile(vShaderCode, fShaderCode, gShaderCode);
            Shaders[name] = shader;
            return shader;
        }

        // retrieves a stored shader
        Shader getShader(const std::string &name)
        {
            return Shaders[name];
        }

        // loads all shaders from a directory
        void loadAllShaders(const std::string &directory)
        {
            for (const auto &entry : std::filesystem::directory_iterator(directory))
            {
                if (entry.is_regular_file())
                {
                    std::string path = entry.path().string();
                    std::string extension = entry.path().extension().string();
                    std::string stem = entry.path().stem().string();

                    if (extension == ".vert")
                    {
                        std::string fPath = directory + "/" + stem + ".frag";
                        std::string gPath = directory + "/" + stem + ".geom";
                        if (std::filesystem::exists(fPath))
                        {
                            if (std::filesystem::exists(gPath))
                            {
                                loadShader(stem, path, fPath, gPath);
                            }
                            else
                            {
                                loadShader(stem, path, fPath);
                            }
                        }
                    }
                }
            }
        }
    };

}
