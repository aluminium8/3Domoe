#include "MITSUDomoe/GuiClient.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include "yyjson.h"

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

GuiClient::GuiClient()
{
    processor->register_cartridge(ReadStlCartridge{});
    processor->register_cartridge(GenerateCentroidsCartridge_mock{});
    processor->register_cartridge(GenerateCentroidsCartridge{});
    processor->register_cartridge(BIGprocess_mock_cartridge{});
    processor->register_cartridge(Need_many_arg_mock_cartridge{});
}

void GuiClient::run()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MITSUDomoe Client", nullptr, nullptr);
    if (window == nullptr) return;
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return;
    glfwSwapInterval(1);

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
    static std::map<std::string, std::string> result_schema;

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
        glfwPollEvents();
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
                        result_schema = success->output_schema;
                    } else if (const auto* error = std::get_if<MITSU_Domoe::ErrorResult>(&result)) {
                        result_json_output = "Error: " + error->error_message;
                        result_schema.clear();
                    }
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("ResultJsonView", ImVec2(0, 0), true);
            ImGui::Text("Result:");
            if (selected_result_id != 0) {
                if (result_schema.empty()) {
                    ImGui::TextWrapped("%s", result_json_output.c_str());
                } else {
                    yyjson_doc *doc = yyjson_read(result_json_output.c_str(), result_json_output.length(), 0);
                    yyjson_val *root = yyjson_doc_get_root(doc);

                    if (root) {
                        for (const auto& pair : result_schema) {
                            const std::string& name = pair.first;
                            const std::string& type = pair.second;

                            yyjson_val *value_val = yyjson_obj_get(root, name.c_str());

                            if (value_val) {
                                const char* value_str = yyjson_val_write(value_val, 0, NULL);
                                if (value_str) {
                                    ImGui::Text("%s [%s]:", name.c_str(), type.c_str());
                                    ImGui::SameLine();
                                    ImGui::TextWrapped("%s", value_str);
                                    free((void*)value_str);
                                }
                            }
                        }
                    }

                    yyjson_doc_free(doc);
                }
            }
            ImGui::EndChild();

            ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
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
