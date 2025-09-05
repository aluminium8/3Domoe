#pragma once

#include <glad/glad.h>
#include <Eigen/Dense>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace MITSU_Domoe {

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
    ~Shader();

    void use();

    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, const Eigen::Vector2f &value) const;
    void setVec3(const std::string &name, const Eigen::Vector3f &value) const;
    void setVec4(const std::string &name, const Eigen::Vector4f &value) const;
    void setMat2(const std::string &name, const Eigen::Matrix2f &mat) const;
    void setMat3(const std::string &name, const Eigen::Matrix3f &mat) const;
    void setMat4(const std::string &name, const Eigen::Matrix4f &mat) const;
    void setDMat4(const std::string &name, const Eigen::Matrix4d &mat) const;

private:
    void checkCompileErrors(GLuint shader, std::string type);
};

}
