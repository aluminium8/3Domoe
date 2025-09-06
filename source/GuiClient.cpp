#include "MITSUDomoe/GuiClient.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include <numbers>
#include "yyjson.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Eigen/Geometry>

// All cartridges used in the app
#include "ReadStlCartridge.hpp"
#include "GenerateCentroidsCartridge_mock.hpp"
#include "GenerateCentroidsCartridge.hpp"
#include "BIGprocess_mock_cartridge.hpp"
#include "Need_many_arg_mock_cartridge.hpp"

#include <rfl.hpp>
#include <rfl_eigen_serdes.hpp>

namespace
{
    static void glfw_error_callback(int error, const char *description)
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

    void GuiClient::process_stl_results()
    {
        auto results = get_all_results();
        for (const auto &pair : results)
        {
            uint64_t id = pair.first;
            if (mesh_render_states.count(id))
            {
                continue; // already processed
            }

            const auto &result = pair.second;
            if (const auto *success = std::get_if<MITSU_Domoe::SuccessResult>(&result))
            {
                if (success->command_name == "readStl")
                {
                    auto output = rfl::json::read<ReadStlCartridge::Output>(success->output_json);
                    if (output)
                    {
                        const auto &mesh = output->polygon_mesh;
                        Eigen::Vector3d min_bound = mesh.V.colwise().minCoeff();
                        Eigen::Vector3d max_bound = mesh.V.colwise().maxCoeff();
                        Eigen::Vector3d center = (min_bound + max_bound) / 2.0;
                        double radius = (max_bound - min_bound).norm() / 2.0;

                        MeshRenderState state;
                        state.renderer = std::make_unique<Renderer>(mesh);
                        state.camera_target = center.cast<float>();
                        state.distance = radius * 2.5f;
                        state.near_clip = 0.01f * radius;
                        state.far_clip = 1000.0f * radius;

                        mesh_render_states[id] = std::move(state);
                    }
                }
            }
        }
    }

    void GuiClient::run()
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return;

        const char *glsl_version = "#version 330";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(1280, 720, "MITSUDomoe Client", nullptr, nullptr);
        if (window == nullptr)
            return;
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            return;
        glfwSwapInterval(1);

        shader_manager.loadAllShaders("shader");

        glEnable(GL_DEPTH_TEST);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        io.ConfigWindowsMoveFromTitleBarOnly = true;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        processor->start();

        auto command_names_vec = get_command_names();
        std::string command_names_str;
        for (const auto &name : command_names_vec)
        {
            command_names_str += name;
            command_names_str += '\0';
        }

        static int selected_command_idx = 0;
        static std::map<std::string, std::string> current_schema;
        static std::map<std::string, std::vector<char>> arg_inputs;
        static uint64_t selected_result_id = 0;
        static std::string result_json_output;
        static std::map<std::string, std::string> result_schema;

        auto update_form_state = [&](const std::string &command_name)
        {
            current_schema = get_input_schema(command_name);
            arg_inputs.clear();
            for (const auto &pair : current_schema)
            {
                arg_inputs[pair.first] = std::vector<char>(256, 0);
            }
        };

        if (!command_names_vec.empty())
        {
            update_form_state(command_names_vec[0]);
        }

        float yaw = -90.0f;
        float pitch = 0.0f;
        float lastX = 1280 / 2.0f;
        float lastY = 720 / 2.0f;
        float fov = 45.0f;
        bool firstMouse = true;
        Eigen::Vector3f cameraPos = Eigen::Vector3f(0.0f, 0.0f, 3.0f);
        Eigen::Vector3f cameraFront = Eigen::Vector3f(0.0f, 0.0f, -1.0f);
        Eigen::Vector3f cameraUp = Eigen::Vector3f(0.0f, 0.0f, 1.0f);

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            process_stl_results();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            {
                ImGui::Begin("Command Sender");

                if (!command_names_vec.empty())
                {
                    if (ImGui::Combo("Command", &selected_command_idx, command_names_str.c_str()))
                    {
                        update_form_state(command_names_vec[selected_command_idx]);
                    }

                    ImGui::Separator();
                    ImGui::Text("Arguments:");

                    for (const auto &pair : current_schema)
                    {
                        const std::string &arg_name = pair.first;
                        const std::string &arg_type = pair.second;
                        ImGui::InputText(arg_name.c_str(), arg_inputs[arg_name].data(), arg_inputs[arg_name].size());
                        ImGui::SameLine();
                        ImGui::TextDisabled("(%s)", arg_type.c_str());
                    }

                    if (ImGui::Button("Submit"))
                    {
                        std::stringstream json_stream;
                        json_stream << "{";
                        bool first = true;
                        for (const auto &pair : arg_inputs)
                        {
                            if (!first)
                                json_stream << ",";

                            const std::string &arg_name = pair.first;
                            const char *arg_value = pair.second.data();
                            const std::string &arg_type = current_schema[arg_name];

                            json_stream << "\"" << arg_name << "\":";

                            if (arg_type.find("string") != std::string::npos || arg_type.find("path") != std::string::npos)
                            {
                                json_stream << "\"" << arg_value << "\"";
                            }
                            else
                            {
                                if (strlen(arg_value) == 0)
                                {
                                    if (arg_type == "bool")
                                        json_stream << "false";
                                    else
                                        json_stream << "0";
                                }
                                else
                                {
                                    json_stream << arg_value;
                                }
                            }
                            first = false;
                        }
                        json_stream << "}";
                        post_command(command_names_vec[selected_command_idx], json_stream.str());
                    }
                }
                else
                {
                    ImGui::Text("No commands registered.");
                }

                ImGui::End();
            }

            {
                ImGui::Begin("Results");
                auto results = get_all_results();
                ImGui::BeginChild("ResultList", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
                for (const auto &pair : results)
                {
                    uint64_t id = pair.first;
                    const auto &result = pair.second;
                    std::string label = "ID: " + std::to_string(id) + " - ";
                    if (const auto *success = std::get_if<MITSU_Domoe::SuccessResult>(&result))
                    {
                        label += success->command_name;
                    }
                    else
                    {
                        label += "Error";
                    }

                    if (ImGui::Selectable(label.c_str(), selected_result_id == id))
                    {
                        selected_result_id = id;
                        if (const auto *success = std::get_if<MITSU_Domoe::SuccessResult>(&result))
                        {
                            result_json_output = success->output_json;
                            result_schema = success->output_schema;
                        }
                        else if (const auto *error = std::get_if<MITSU_Domoe::ErrorResult>(&result))
                        {
                            result_json_output = "Error: " + error->error_message;
                            result_schema.clear();
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("ResultJsonView", ImVec2(0, 0), true, ImGuiWindowFlags_NoMove);
                ImGui::Text("Result:");
                if (selected_result_id != 0)
                {
                    if (result_schema.empty())
                    {
                        ImGui::TextWrapped("%s", result_json_output.c_str());
                    }
                    else
                    {
                        yyjson_doc *doc = yyjson_read(result_json_output.c_str(), result_json_output.length(), 0);
                        yyjson_val *root = yyjson_doc_get_root(doc);

                        if (root)
                        {
                            if (ImGui::BeginTable("result_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
                            {
                                ImGui::TableSetupColumn("Field");
                                ImGui::TableSetupColumn("Value");
                                ImGui::TableSetupColumn("Reference", ImGuiTableColumnFlags_WidthFixed);
                                ImGui::TableHeadersRow();

                                for (const auto &pair : result_schema)
                                {
                                    const std::string &name = pair.first;
                                    const std::string &type = pair.second;
                                    yyjson_val *value_val = yyjson_obj_get(root, name.c_str());
                                    if (value_val)
                                    {
                                        const char *value_str = yyjson_val_write(value_val, 0, NULL);
                                        if (value_str)
                                        {
                                            ImGui::TableNextRow();
                                            ImGui::TableSetColumnIndex(0);
                                            ImGui::TextUnformatted((name + "\n [" + type + "]").c_str());
                                            ImGui::TableSetColumnIndex(1);
                                            ImGui::PushID(name.c_str());
                                            ImGui::TextWrapped("%s", value_str);
                                            if (ImGui::IsItemClicked())
                                            {
                                                ImGui::SetClipboardText(value_str);
                                                ImGui::OpenPopup("CopiedPopup");
                                            }
                                            ImGui::PopID();
                                            ImGui::TableSetColumnIndex(2);
                                            std::string button_label = "Ref##" + name;
                                            if (ImGui::Button(button_label.c_str()))
                                            {
                                                std::string ref_syntax = "$ref:cmd[" + std::to_string(selected_result_id) + "]." + name;
                                                ImGui::SetClipboardText(ref_syntax.c_str());
                                                ImGui::OpenPopup("CopiedPopup");
                                            }
                                            free((void *)value_str);
                                        }
                                    }
                                }
                                if (mesh_render_states.count(selected_result_id))
                                {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Render");
                                    ImGui::TableSetColumnIndex(1);
                                    auto &state = mesh_render_states.at(selected_result_id);
                                    ImGui::Checkbox("Visible", &state.is_visible);
                                    std::string shader_names_str;
                                    std::vector<std::string> shader_names_vec;
                                    for (const auto &pair : shader_manager.Shaders)
                                    {
                                        shader_names_str += pair.first + '\0';
                                        shader_names_vec.push_back(pair.first);
                                    }
                                    int current_shader_idx = -1;
                                    for (int i = 0; i < shader_names_vec.size(); ++i)
                                    {
                                        if (shader_names_vec[i] == state.selected_shader_name)
                                        {
                                            current_shader_idx = i;
                                            break;
                                        }
                                    }
                                    if (ImGui::Combo("Shader", &current_shader_idx, shader_names_str.c_str()))
                                    {
                                        state.selected_shader_name = shader_names_vec[current_shader_idx];
                                    }
                                }
                                ImGui::EndTable();
                            }
                        }
                        yyjson_doc_free(doc);
                    }
                }
                ImGui::EndChild();
                if (ImGui::BeginPopup("CopiedPopup"))
                {
                    ImGui::Text("Copied to clipboard!");
                    ImGui::EndPopup();
                }
                ImGui::End();
            }

            // Rendering
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Camera/View transformation
            Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
            Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
            Eigen::Vector3f current_camera_pos = cameraPos;
            Eigen::Vector3f current_camera_target = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
            float near = 0.1f;
            float far = 100.0f;

            if (mesh_render_states.count(selected_result_id))
            {
                auto &state = mesh_render_states.at(selected_result_id);
                if (!io.WantCaptureMouse)
                {
                    if (io.MouseWheel != 0.0f)
                    {
                        state.distance -= io.MouseWheel * 0.1f * state.distance;
                        if (state.distance < 0.1f)
                            state.distance = 0.1f;
                    }

                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                        state.yaw -= delta.x * 0.25f;
                        state.pitch -= delta.y * 0.25f;
                        if (state.pitch > 89.0f)
                            state.pitch = 89.0f;
                        if (state.pitch < -89.0f)
                            state.pitch = -89.0f;
                    }
                }

                current_camera_target = state.camera_target;
                near = state.near_clip;
                far = state.far_clip;

                Eigen::Vector3f dir;
                dir.x() = cos(state.yaw * std::numbers::pi / 180.0f) * cos(state.pitch * std::numbers::pi / 180.0f);
                dir.y() = sin(state.yaw * std::numbers::pi / 180.0f) * cos(state.pitch * std::numbers::pi / 180.0f);
                dir.z() = sin(state.pitch * std::numbers::pi / 180.0f);
                current_camera_pos = state.camera_target - dir.normalized() * state.distance;

                Eigen::Vector3f z_axis = (current_camera_pos - current_camera_target).normalized();
                Eigen::Vector3f x_axis = cameraUp.cross(z_axis).normalized();
                Eigen::Vector3f y_axis = z_axis.cross(x_axis);
                view(0, 0) = x_axis.x();
                view(0, 1) = x_axis.y();
                view(0, 2) = x_axis.z();
                view(0, 3) = -x_axis.dot(current_camera_pos);
                view(1, 0) = y_axis.x();
                view(1, 1) = y_axis.y();
                view(1, 2) = y_axis.z();
                view(1, 3) = -y_axis.dot(current_camera_pos);
                view(2, 0) = z_axis.x();
                view(2, 1) = z_axis.y();
                view(2, 2) = z_axis.z();
                view(2, 3) = -z_axis.dot(current_camera_pos);
                view(3, 0) = 0;
                view(3, 1) = 0;
                view(3, 2) = 0;
                view(3, 3) = 1.0f;
            }

            float aspect = (float)display_w / (float)display_h;
            float top = near * tan(fov * 0.5f * std::numbers::pi / 180.0f);
            float bottom = -top;
            float right = top * aspect;
            float left = -right;
            projection(0, 0) = (2.0f * near) / (right - left);
            projection(1, 1) = (2.0f * near) / (top - bottom);
            projection(0, 2) = (right + left) / (right - left);
            projection(1, 2) = (top + bottom) / (top - bottom);
            projection(2, 2) = -(far + near) / (far - near);
            projection(3, 2) = -1.0f;
            projection(2, 3) = -(2.0f * far * near) / (far - near);
            projection(3, 3) = 0.0f;

            for (auto const &[id, state] : mesh_render_states)
            {
                if (state.is_visible)
                {
                    Shader shader = shader_manager.getShader(state.selected_shader_name);
                    shader.use();
                    shader.setMatrix4fv("projection", projection);
                    shader.setMatrix4fv("view", view);
                    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
                    shader.setMatrix4fv("model", model);
                    state.renderer->draw(shader);
                }
            }

            ImGui::Render();
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
