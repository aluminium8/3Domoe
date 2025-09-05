#include "MITSUDomoe/GuiClient.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

GuiClient::GuiClient()
{
    processor->register_cartridge(ReadStlCartridge{});
    processor->register_cartridge(GenerateCentroidsCartridge_mock{});
    processor->register_cartridge(GenerateCentroidsCartridge{});
    processor->register_cartridge(BIGprocess_mock_cartridge{});
    processor->register_cartridge(Need_many_arg_mock_cartridge{});
}

GuiClient::~GuiClient()
{
    cleanup();
}

void GuiClient::init_glfw()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    const char* glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1280, 720, "MITSUDomoe Client", nullptr, nullptr);
    if (window == nullptr)
        throw std::runtime_error("Failed to create GLFW window");

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");

    glfwSwapInterval(1);
}

void GuiClient::init_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
}

void GuiClient::load_shaders()
{
    try {
        shaders["Edge Draw"] = std::make_unique<Shader>("./shader/test_shader_with_mvp_edge_draw.vert", "./shader/test_shader_with_mvp_edge_draw.frag", "./shader/test_shader_with_mvp_edge_draw.geom");
        shaders["Solid"] = std::make_unique<Shader>("./shader/solid.vert", "./shader/solid.frag");
        std::cout << "Shaders loaded successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR::SHADER::LOADING_FAILED\n" << e.what() << std::endl;
    }
}

void GuiClient::main_loop()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

    const char* shader_items[] = { "Edge Draw", "Solid" };
    static int current_shader_idx = 0;

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 3D rendering
        if (model_renderer) {
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 100.0f);
            glm::mat4 view = glm::lookAt(glm::vec3(1.5, 1.5, 1.5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            glm::mat4 model = glm::mat4(1.0f);

            Eigen::Matrix4d mvp_eigen;
            // GLM matrices are column-major, Eigen defaults to column-major.
            // Direct conversion should be fine.
            // The shaders expect a dmat4, so we construct an Eigen::Matrix4d
            glm::mat4 mvp_glm = projection * view * model;
            // The shaders I wrote expect a dmat4, but the solid shader expects a mat4.
            // For now, I will convert to double for the edge shader and float for the solid shader.

            std::string selected_shader_name = shader_items[current_shader_idx];
            Shader* current_shader = shaders[selected_shader_name].get();

            if (current_shader) {
                current_shader->use();
                if (selected_shader_name == "Edge Draw") {
                    Eigen::Matrix4d mvp_eigen_double;
                    memcpy(mvp_eigen_double.data(), glm::value_ptr(mvp_glm), sizeof(mvp_glm));
                    current_shader->setDMat4("mvp", mvp_eigen_double);
                } else {
                    Eigen::Matrix4f mvp_eigen_float;
                    memcpy(mvp_eigen_float.data(), glm::value_ptr(mvp_glm), sizeof(mvp_glm));
                    current_shader->setMat4("mvp", mvp_eigen_float);
                }
                model_renderer->draw(*current_shader);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- ImGui Windows ---
        ImGui::Begin("Viewport");
        ImGui::Combo("Shader", &current_shader_idx, shader_items, IM_ARRAYSIZE(shader_items));
        ImGui::End();

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
                        if (success->command_name == "readStl") {
                            try {
                                const auto& stl_output = std::any_cast<const ReadStlCartridge::Output&>(success->output_raw);
                                model_renderer = std::make_unique<ModelRenderer>(stl_output.polygon_mesh);
                                std::cout << "Successfully created ModelRenderer for the selected STL mesh." << std::endl;
                            } catch (const std::bad_any_cast& e) {
                                std::cout << "Cast failed for readStl output. " << e.what() << std::endl;
                                model_renderer.reset();
                            }
                        } else {
                            std::cout << "Selected result is not from readStl, clearing renderer." << std::endl;
                            model_renderer.reset();
                        }
                    } else if (const auto* error = std::get_if<MITSU_Domoe::ErrorResult>(&result)) {
                        result_json_output = "Error: " + error->error_message;
                        model_renderer.reset();
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

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void GuiClient::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void GuiClient::run()
{
    try {
        init_glfw();
        init_imgui();
        processor->start();
        load_shaders();
        main_loop();
    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }
}

}
