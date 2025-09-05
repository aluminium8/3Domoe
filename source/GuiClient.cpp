#define _USE_MATH_DEFINES
#include <cmath>
#include "MITSUDomoe/GuiClient.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <sstream>
#include <cstring>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// All cartridges used in the app
#include "ReadStlCartridge.hpp"
#include "GenerateCentroidsCartridge_mock.hpp"
#include "GenerateCentroidsCartridge.hpp"
#include "BIGprocess_mock_cartridge.hpp"
#include "Need_many_arg_mock_cartridge.hpp"


namespace
{
    static void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }
}

namespace MITSU_Domoe
{

namespace {
    Eigen::Matrix4f perspective(float fovY, float aspect, float zNear, float zFar) {
        float tanHalfFovy = tan(fovY / 2.0f);
        Eigen::Matrix4f res = Eigen::Matrix4f::Zero();
        res(0, 0) = 1.0f / (aspect * tanHalfFovy);
        res(1, 1) = 1.0f / (tanHalfFovy);
        res(2, 2) = -(zFar + zNear) / (zFar - zNear);
        res(3, 2) = -1.0f;
        res(2, 3) = -(2.0f * zFar * zNear) / (zFar - zNear);
        return res;
    }
}


void GuiClient::ProcessMouseMovement(double xpos, double ypos) {
    if (ImGui::IsMouseDown(0)) { // Only move camera if left mouse button is down
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    } else {
        firstMouse = true;
    }
}

void GuiClient::ProcessMouseScroll(double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}


GuiClient::GuiClient() : camera(Eigen::Vector3f(0.0f, 0.0f, 3.0f))
{
    processor->register_cartridge(ReadStlCartridge{});
    processor->register_cartridge(GenerateCentroidsCartridge_mock{});
    processor->register_cartridge(GenerateCentroidsCartridge{});
    processor->register_cartridge(BIGprocess_mock_cartridge{});
    processor->register_cartridge(Need_many_arg_mock_cartridge{});

    shaders["Edge Shader"] = std::make_unique<Shader>("shader/test_shader_with_mvp_edge_draw.vert", "shader/test_shader_with_mvp_edge_draw.frag", "shader/test_shader_with_mvp_edge_draw.geom");
    shaders["Normal Color Shader"] = std::make_unique<Shader>("shader/normal_color.vert", "shader/normal_color.frag", "shader/normal_color.geom");
    current_shader_name = "Edge Shader";
}

GuiClient::~GuiClient() {

}

void GuiClient::process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void GuiClient::run()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;

    const char* glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(1280, 720, "MITSUDomoe Client", nullptr, nullptr);
    if (window == nullptr) return;
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return;
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);

    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1280, 720);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    processor->start();

    auto command_names_vec = get_command_names();
    std::string command_names_str;
    for(const auto& name : command_names_vec) {
        command_names_str += name;
        command_names_str += '\0';
    }

    static int selected_command_idx = 0;
    static std::map<std::string, std::string> current_schema;
    static std::map<std::string, std::vector<char>> arg_inputs;
    static uint64_t selected_result_id = 0;
    static std::string result_json_output;

    auto update_form_state = [&](const std::string& command_name) {
        current_schema = get_input_schema(command_name);
        arg_inputs.clear();
        for (const auto& pair : current_schema) {
            arg_inputs[pair.first] = std::vector<char>(256, 0);
        }
    };

    if (!command_names_vec.empty()) {
        update_form_state(command_names_vec[0]);
    }

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        process_input(window);

        glfwPollEvents();

        // Check for new results and create renderable meshes
        auto results = get_all_results();
        for (const auto& pair : results) {
            uint64_t id = pair.first;
            if (renderable_meshes.find(id) == renderable_meshes.end()) {
                const auto& result = pair.second;
                if (const auto* success = std::get_if<MITSU_Domoe::SuccessResult>(&result)) {
                    if (success->command_name == "readStl") {
                        // This is a bit of a hack, we should probably have a better way to get the mesh
                        // For now, we deserialize it from the JSON output.
                        rfl::Result<ReadStlCartridge::Output> output = rfl::json::read<ReadStlCartridge::Output>(success->output_json);
                        if(output) {
                            renderable_meshes[id] = std::make_unique<RenderableMesh>(output->polygon_mesh);
                        }
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("Command Sender");

            if (!command_names_vec.empty()) {
                if (ImGui::Combo("Command", &selected_command_idx, command_names_str.c_str())) {
                    update_form_state(command_names_vec[selected_command_idx]);
                }

                ImGui::Separator();
                ImGui::Text("Arguments:");

                for (const auto& pair : current_schema) {
                    const std::string& arg_name = pair.first;
                    const std::string& arg_type = pair.second;
                    ImGui::InputText(arg_name.c_str(), arg_inputs[arg_name].data(), arg_inputs[arg_name].size());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%s)", arg_type.c_str());
                }

                if (ImGui::Button("Submit")) {
                    std::stringstream json_stream;
                    json_stream << "{";
                    bool first = true;
                    for (const auto& pair : arg_inputs) {
                        if (!first) json_stream << ",";

                        const std::string& arg_name = pair.first;
                        const char* arg_value = pair.second.data();
                        const std::string& arg_type = current_schema[arg_name];

                        json_stream << "\"" << arg_name << "\":";

                        if (arg_type.find("string") != std::string::npos || arg_type.find("path") != std::string::npos) {
                            json_stream << "\"" << arg_value << "\"";
                        } else {
                            if (strlen(arg_value) == 0) {
                                if (arg_type == "bool") json_stream << "false";
                                else json_stream << "0";
                            } else {
                                json_stream << arg_value;
                            }
                        }
                        first = false;
                    }
                    json_stream << "}";
                    post_command(command_names_vec[selected_command_idx], json_stream.str());
                }

            } else {
                ImGui::Text("No commands registered.");
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Results");
            auto results = get_all_results();
            ImGui::BeginChild("ResultList", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
            for (const auto& pair : results) {
                uint64_t id = pair.first;
                const auto& result = pair.second;
                std::string label = "ID: " + std::to_string(id) + " - ";
                if (const auto* success = std::get_if<MITSU_Domoe::SuccessResult>(&result)) {
                    label += success->command_name;
                } else {
                    label += "Error";
                }

                if (ImGui::Selectable(label.c_str(), selected_result_id == id)) {
                    selected_result_id = id;
                    if (const auto* success = std::get_if<MITSU_Domoe::SuccessResult>(&result)) {
                        result_json_output = success->output_json;
                    } else if (const auto* error = std::get_if<MITSU_Domoe::ErrorResult>(&result)) {
                        result_json_output = "Error: " + error->error_message;
                    }
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("ResultJsonView", ImVec2(0, 0), true);
            ImGui::Text("Result JSON:");
            ImGui::TextWrapped("%s", result_json_output.c_str());
            ImGui::EndChild();

            ImGui::End();
        }

        // 3D Scene Rendering
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (shaders.count(current_shader_name)) {
            auto& shader = shaders.at(current_shader_name);
            shader->use();

            Eigen::Matrix4f proj = perspective(camera.Zoom * M_PI / 180.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
            Eigen::Matrix4d view = camera.GetViewMatrixDouble();
            Eigen::Matrix4d model = Eigen::Matrix4d::Identity();
            Eigen::Matrix4d mvp = proj.cast<double>() * view * model;

            shader->setDMat4("mvp", mvp);

            for (auto const& [id, mesh] : renderable_meshes) {
                mesh->draw(*shader);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ImGui Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        {
            ImGui::Begin("3D Viewport");

            if (ImGui::IsWindowHovered()) {
                ImGuiIO& io = ImGui::GetIO();
                ProcessMouseScroll(io.MouseWheel);
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                ProcessMouseMovement(xpos, ypos);
            }

            ImGui::BeginChild("ShaderSelector");
            const char* current = current_shader_name.c_str();
            if (ImGui::BeginCombo("Shader", current)) {
                for (auto const& [name, shader] : shaders) {
                    bool is_selected = (current_shader_name == name);
                    if (ImGui::Selectable(name.c_str(), is_selected)) {
                        current_shader_name = name;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::EndChild();
            ImGui::Image((ImTextureID)(intptr_t)textureColorbuffer, ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::End();
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

}
