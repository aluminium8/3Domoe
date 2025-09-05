#pragma once

#include "BaseClient.hpp"
#include "Shader.hpp"
#include <map>
#include <string>
#include <memory>

#include "ModelRenderer.hpp"

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
    void init_glfw();
    void init_imgui();
    void load_shaders();
    void main_loop();
    void cleanup();

    GLFWwindow* window = nullptr;
    std::map<std::string, std::unique_ptr<Shader>> shaders;
    std::unique_ptr<ModelRenderer> model_renderer;
};

}
