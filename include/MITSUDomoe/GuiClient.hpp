#pragma once

#include "BaseClient.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "RenderableMesh.hpp"

#include <map>
#include <string>
#include <memory>

struct GLFWwindow;

namespace MITSU_Domoe
{

class GuiClient : public BaseClient
{
public:
    GuiClient();
    ~GuiClient();

    void run() override;

private:
    void process_input(GLFWwindow* window);
    void ProcessMouseMovement(double xpos, double ypos);
    void ProcessMouseScroll(double yoffset);


    Camera camera;
    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;
    bool firstMouse = true;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    std::map<std::string, std::unique_ptr<Shader>> shaders;
    std::string current_shader_name;

    std::map<uint64_t, std::unique_ptr<RenderableMesh>> renderable_meshes;
};

}
