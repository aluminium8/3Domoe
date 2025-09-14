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
#include "CutMeshCartridge.hpp"
#include "SubdividePolygonCartridge.hpp"


#include <rfl.hpp>
#include <rfl_eigen_serdes.hpp>
#include <fstream>

namespace
{
    static void glfw_error_callback(int error, const char *description)
    {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }
}

namespace MITSU_Domoe
{
    void GuiClient::render_json_tree_node(yyjson_val *node, const std::string &current_path, uint64_t result_id)
    {
        if (!node)
            return;

        // For each node, we create a line with a Ref button, the node's name (or index), and its value or a collapsible tree.
        ImGui::PushID(current_path.c_str());

        // Extract the last part of the path for the label
        std::string label = current_path;
        size_t last_dot = current_path.find_last_of('.');
        if (last_dot != std::string::npos) {
            label = current_path.substr(last_dot + 1);
        }

        // Handle root case
        if (current_path.empty()) {
            label = "root";
        }

        bool is_container = yyjson_is_obj(node) || yyjson_is_arr(node);

        // For containers, we make the node collapsible. For primitives, we just display the value.
        if (is_container)
        {
            bool tree_node_open = ImGui::TreeNode(label.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Ref"))
            {
                std::string ref_syntax = "$ref:cmd[" + std::to_string(result_id) + "]" + (current_path.empty() ? "" : ".") + current_path;
                ImGui::SetClipboardText(ref_syntax.c_str());
                ImGui::OpenPopup("CopiedPopup");
            }

            if (tree_node_open)
            {
                if (yyjson_is_obj(node))
                {
                    yyjson_obj_iter iter;
                    yyjson_obj_iter_init(node, &iter);
                    yyjson_val *key, *val;
                    while ((key = yyjson_obj_iter_next(&iter)))
                    {
                        val = yyjson_obj_iter_get_val(key);
                        const char *key_str = yyjson_get_str(key);
                        std::string new_path = current_path.empty() ? key_str : current_path + "." + key_str;
                        render_json_tree_node(val, new_path, result_id);
                    }
                }
                else
                { // is_arr
                    size_t idx, max;
                    yyjson_val *val;
                    yyjson_arr_foreach(node, idx, max, val)
                    {
                        std::string new_path = current_path + "[" + std::to_string(idx) + "]";
                        render_json_tree_node(val, new_path, result_id);
                    }
                }
                ImGui::TreePop();
            }
        }
        else
        {
            // For primitive types, display label, value and Ref button on one line.
            ImGui::Bullet();
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();

            const char *value_str = yyjson_val_write(node, 0, NULL);
            ImGui::TextDisabled("%s", value_str);
            free((void *)value_str);

            ImGui::SameLine();
            if (ImGui::Button("Ref"))
            {
                std::string ref_syntax = "$ref:cmd[" + std::to_string(result_id) + "]" + (current_path.empty() ? "" : ".") + current_path;
                ImGui::SetClipboardText(ref_syntax.c_str());
                ImGui::OpenPopup("CopiedPopup");
            }
        }

        ImGui::PopID();
    }

    GuiClient::GuiClient(const std::filesystem::path &log_path) : BaseClient(log_path)
    {
        processor->register_cartridge(ReadStlCartridge{});
        processor->register_cartridge(GenerateCentroidsCartridge_mock{});
        processor->register_cartridge(GenerateCentroidsCartridge{});
        processor->register_cartridge(BIGprocess_mock_cartridge{});
        processor->register_cartridge(Need_many_arg_mock_cartridge{});
        processor->register_cartridge(CutMeshCartridge{});
        processor->register_cartridge(SubdividePolygonCartridge{});
        
    }

    void GuiClient::process_mesh_results()
    {
        auto results = get_all_results();
        for (const auto &pair : results)
        {
            uint64_t id = pair.first;
            const auto &result = pair.second;

            if (const auto *success =
                    std::get_if<MITSU_Domoe::SuccessResult>(&result))
            {
                for (const auto &output_pair : success->output_schema)
                {
                    const std::string &output_name = output_pair.first;
                    const std::string &output_type = output_pair.second;

                    auto mesh_key = std::make_pair(id, output_name);
                    if (mesh_render_states.count(mesh_key))
                    {
                        continue; // already processed
                    }

                    if (output_type.find("Polygon_mesh") != std::string::npos)
                    {
                        yyjson_doc *doc = yyjson_read(success->output_json.c_str(),
                                                      success->output_json.length(), 0);
                        yyjson_val *root = yyjson_doc_get_root(doc);
                        yyjson_val *mesh_val = yyjson_obj_get(root, output_name.c_str());
                        if (!mesh_val)
                        {
                            yyjson_doc_free(doc);
                            continue;
                        }
                        const char *mesh_json = yyjson_val_write(mesh_val, 0, NULL);

                        auto mesh_obj =
                            rfl::json::read<MITSU_Domoe::Polygon_mesh>(mesh_json);

                        free((void *)mesh_json);
                        yyjson_doc_free(doc);

                        if (mesh_obj)
                        {
                            const auto &mesh = *mesh_obj;
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

                            mesh_render_states[mesh_key] = std::move(state);
                        }
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
        static std::string unresolved_input_for_display;
        static std::string resolved_input_for_display;

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

            process_mesh_results();

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

                ImGui::Separator();
                ImGui::Text("Log Playback");
                ImGui::InputText("Log Path", log_path_buffer, sizeof(log_path_buffer));
                if (ImGui::Button("Load Log"))
                {
                    handle_load(log_path_buffer);
                }
                ImGui::SameLine();
                if (ImGui::Button("Trace Log"))
                {
                    handle_trace(log_path_buffer);
                }
                ImGui::SameLine();
                if (ImGui::Button("Trace Command History"))
                {
                    handle_trace_history(log_path_buffer);
                }

                if (!loaded_log_content.empty())
                {
                    ImGui::Separator();
                    ImGui::Text("Loaded Log Content:");
                    ImGui::InputTextMultiline("##loaded_log", &loaded_log_content[0], loaded_log_content.size(), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);
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
                            unresolved_input_for_display = success->unresolved_input_json;
                            resolved_input_for_display = success->resolved_input_json;
                        }
                        else if (const auto *error = std::get_if<MITSU_Domoe::ErrorResult>(&result))
                        {
                            result_json_output = "Error: " + error->error_message;
                            result_schema.clear();
                            unresolved_input_for_display = "N/A";
                            resolved_input_for_display = "N/A";
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
                            render_json_tree_node(root, "", selected_result_id);

                            // Render controls for each mesh
                            ImGui::Separator();
                            ImGui::Text("Render Controls");
                            for (const auto &schema_pair : result_schema)
                            {
                                const std::string &output_name = schema_pair.first;
                                const std::string &output_type = schema_pair.second;

                                if (output_type.find("Polygon_mesh") != std::string::npos)
                                {
                                    auto mesh_key = std::make_pair(selected_result_id, output_name);
                                    if (mesh_render_states.count(mesh_key))
                                    {
                                        auto &state = mesh_render_states.at(mesh_key);
                                        ImGui::PushID(output_name.c_str());
                                        ImGui::Text("Mesh: %s", output_name.c_str());
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
                                        ImGui::PopID();
                                    }
                                }
                            }
                        }
                        yyjson_doc_free(doc);
                    }

                    ImGui::Separator();
                    ImGui::Text("Unresolved Input:");
                    ImGui::InputTextMultiline("##unresolved_input", &unresolved_input_for_display[0], unresolved_input_for_display.size(), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4), ImGuiInputTextFlags_ReadOnly);
                    ImGui::Text("Resolved Input:");
                    ImGui::InputTextMultiline("##resolved_input", &resolved_input_for_display[0], resolved_input_for_display.size(), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4), ImGuiInputTextFlags_ReadOnly);
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

            // Find the first mesh associated with the selected result to control the camera
            MITSU_Domoe::MeshRenderState *camera_target_state = nullptr;
            for (auto &[key, state] : mesh_render_states)
            {
                if (key.first == selected_result_id)
                {
                    camera_target_state = &state;
                    break;
                }
            }

            if (camera_target_state)
            {
                auto &state = *camera_target_state;
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

            for (auto const &[mesh_key, state] : mesh_render_states)
            {
                if (state.is_visible)
                {
                    Shader shader = shader_manager.getShader(state.selected_shader_name);
                    shader.use();
                    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
                    Eigen::Matrix4f mvp = projection * view * model;
                    shader.setMatrix4fv("MVP", mvp);
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

    void GuiClient::handle_load(const std::string &path_str)
    {
        const std::filesystem::path path(path_str);
        loaded_log_content.clear();

        auto process_file = [this](const std::filesystem::path &file_path)
        {
            if (file_path.extension() != ".json")
            {
                return;
            }
            spdlog::info("GUI: Loading log: {}", file_path.string());
            std::ifstream ifs(file_path);
            if (!ifs)
            {
                spdlog::error("Failed to open file: {}", file_path.string());
                return;
            }
            const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

            // Store the result from the log in the repository
            this->load_result(content);

            try
            {
                auto parsed = rfl::json::read<rfl::Generic>(content);
                if (!parsed)
                {
                    throw std::runtime_error(parsed.error().what());
                }
                const auto pretty_json = rfl::json::write(*parsed, YYJSON_WRITE_PRETTY);
                loaded_log_content += "--- " + file_path.filename().string() + " ---\n";
                loaded_log_content += pretty_json;
                loaded_log_content += "\n\n";
            }
            catch (const std::exception &e)
            {
                spdlog::error("Failed to parse or print JSON from {}: {}", file_path.string(), e.what());
            }
        };

        if (std::filesystem::is_directory(path))
        {
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    process_file(entry.path());
                }
            }
        }
        else if (std::filesystem::is_regular_file(path))
        {
            process_file(path);
        }
        else
        {
            spdlog::error("Path is not a valid file or directory: {}", path_str);
        }
    }

    void GuiClient::handle_trace(const std::string &path_str)
    {
        const std::filesystem::path path(path_str);

        auto process_file = [this](const std::filesystem::path &file_path)
        {
            if (file_path.extension() != ".json")
            {
                return;
            }
            spdlog::info("GUI: Tracing log: {}", file_path.string());
            std::ifstream ifs(file_path);
            if (!ifs)
            {
                spdlog::error("Failed to open file: {}", file_path.string());
                return;
            }
            const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

            try
            {
                struct Log
                {
                    rfl::Field<"command", std::string> command;
                    rfl::Field<"request", rfl::Generic> request;
                };

                auto parsed = rfl::json::read<Log>(content);
                if (!parsed)
                {
                    throw std::runtime_error(parsed.error().what());
                }

                const std::string &command_name = parsed->command();
                const std::string request_json = rfl::json::write(parsed->request());

                spdlog::info("GUI: Re-posting command '{}' with input: {}", command_name, request_json);
                this->post_command(command_name, request_json);
            }
            catch (const std::exception &e)
            {
                spdlog::error("Failed to parse or trace JSON from {}: {}", file_path.string(), e.what());
            }
        };

        if (std::filesystem::is_directory(path))
        {
            std::vector<std::filesystem::path> files;
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    files.push_back(entry.path());
                }
            }
            std::sort(files.begin(), files.end());
            for (const auto &file_path : files)
            {
                process_file(file_path);
            }
        }
        else if (std::filesystem::is_regular_file(path))
        {
            process_file(path);
        }
        else
        {
            spdlog::error("Path is not a valid file or directory: {}", path_str);
        }
    }

    void GuiClient::handle_trace_history(const std::string &path_str)
    {
        const std::filesystem::path path(path_str);

        auto process_file = [this](const std::filesystem::path &file_path)
        {
            if (file_path.extension() != ".json")
            {
                return;
            }
            spdlog::info("GUI: Tracing command history log: {}", file_path.string());
            std::ifstream ifs(file_path);
            if (!ifs)
            {
                spdlog::error("Failed to open file: {}", file_path.string());
                return;
            }
            const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

            try
            {
                struct Log
                {
                    rfl::Field<"command", std::string> command;
                    rfl::Field<"request", rfl::Generic> request;
                };

                auto parsed = rfl::json::read<Log>(content);
                if (!parsed)
                {
                    throw std::runtime_error(parsed.error().what());
                }

                const std::string &command_name = parsed->command();
                const std::string request_json = rfl::json::write(parsed->request());

                spdlog::info("GUI: Re-posting command '{}' from history with input: {}", command_name, request_json);
                this->post_command(command_name, request_json);
            }
            catch (const std::exception &e)
            {
                spdlog::error("Failed to parse or trace JSON from {}: {}", file_path.string(), e.what());
            }
        };

        if (std::filesystem::is_directory(path))
        {
            std::vector<std::filesystem::path> files;
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    files.push_back(entry.path());
                }
            }
            std::sort(files.begin(), files.end());
            for (const auto &file_path : files)
            {
                process_file(file_path);
            }
        }
        else
        {
            spdlog::error("Path is not a valid directory for tracing history: {}", path_str);
        }
    }
}
