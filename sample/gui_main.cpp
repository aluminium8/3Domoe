#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <thread>

#include <MITSUDomoe/CommandProcessor.hpp>
#include <MITSUDomoe/ResultRepository.hpp>
#include <MITSUDomoe/ICartridge.hpp>

// All cartridges used in the app
#include "ReadStlCartridge.hpp"
#include "GenerateCentroidsCartridge.hpp"
#include "BIGprocess_mock_cartridge.hpp"
#include "Need_many_arg_mock_cartridge.hpp"


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "3Domoe GUI Client", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // --- Application State ---
    auto result_repo = std::make_shared<MITSU_Domoe::ResultRepository>();
    MITSU_Domoe::CommandProcessor processor(result_repo);

    // Register cartridges
    processor.register_cartridge(ReadStlCartridge{});
    processor.register_cartridge(GenerateCentroidsCartridge{});
    processor.register_cartridge(BIGprocess_mock_cartridge{});
    processor.register_cartridge(Need_many_arg_mock_cartridge{});

    processor.start();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- GUI Code ---

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 display_size = viewport->WorkSize;

        // Command Launcher Window (Top 40%)
        {
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(ImVec2(display_size.x, display_size.y * 0.4f));
            ImGui::Begin("Command Launcher", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            static std::vector<std::string> command_names;
            if (command_names.empty()) {
                command_names = processor.get_registered_command_names();
            }

            static int selected_command_idx = 0;
            const char* current_command_name = command_names[selected_command_idx].c_str();
            if (ImGui::BeginCombo("Command", current_command_name)) {
                for (int n = 0; n < command_names.size(); n++) {
                    const bool is_selected = (selected_command_idx == n);
                    if (ImGui::Selectable(command_names[n].c_str(), is_selected))
                        selected_command_idx = n;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            static char json_input[1024] = "{\n  \"filepath\": \"sample/resource/tetra.stl\"\n}";
            ImGui::InputTextMultiline("JSON Input", json_input, IM_ARRAYSIZE(json_input), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8));

            if (ImGui::Button("Dispatch Command")) {
                processor.add_to_queue(command_names[selected_command_idx], json_input);
            }

            ImGui::End();
        }

        // Command History and Results Window (Bottom 60%)
        {
            ImVec2 second_window_pos = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + display_size.y * 0.4f);
            ImVec2 second_window_size = ImVec2(display_size.x, display_size.y * 0.6f);
            ImGui::SetNextWindowPos(second_window_pos);
            ImGui::SetNextWindowSize(second_window_size);
            ImGui::Begin("History & Inspector", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            static std::vector<uint64_t> history;
            // Get the range of issued command IDs
            uint64_t next_id = processor.get_next_command_id();
            if (next_id > history.size() + 1) {
                 for(uint64_t i = history.size() + 1; i < next_id; ++i) {
                    history.push_back(i);
                 }
            }


            const int COLUMN_COUNT = 3;
            static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

            if (ImGui::BeginTable("HistoryTable", COLUMN_COUNT, flags))
            {
                ImGui::TableSetupColumn("ID");
                ImGui::TableSetupColumn("Command");
                ImGui::TableSetupColumn("Status");
                ImGui::TableHeadersRow();

                static int selected_row = -1;
                for (int row = 0; row < history.size(); row++)
                {
                    ImGui::TableNextRow();
                    uint64_t id = history[row];
                    auto result = result_repo->get_result(id);

                    ImGui::TableSetColumnIndex(0);
                    char label[32];
                    sprintf(label, "%lu", id);
                    if (ImGui::Selectable(label, selected_row == row, ImGuiSelectableFlags_SpanAllColumns)) {
                        selected_row = row;
                    }

                    ImGui::TableSetColumnIndex(1);
                    if(result) {
                         if (const auto* success = std::get_if<MITSU_Domoe::SuccessResult>(&(*result))) {
                            ImGui::Text("%s", success->command_name.c_str());
                         } else if (const auto* error = std::get_if<MITSU_Domoe::ErrorResult>(&(*result))) {
                            ImGui::Text("%s", error->command_name.c_str());
                         }
                    } else {
                        ImGui::Text("N/A");
                    }


                    ImGui::TableSetColumnIndex(2);
                     if (result) {
                        if (std::get_if<MITSU_Domoe::SuccessResult>(&(*result))) {
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Success");
                        } else if (std::get_if<MITSU_Domoe::ErrorResult>(&(*result))) {
                             ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error");
                        }
                    } else {
                        ImGui::Text("Running...");
                    }
                }
                ImGui::EndTable();

                ImGui::Separator();
                ImGui::Text("Result Inspector");

                if (selected_row != -1) {
                    uint64_t id = history[selected_row];
                    auto result = result_repo->get_result(id);
                    if (result) {
                        if (const auto* success = std::get_if<MITSU_Domoe::SuccessResult>(&(*result))) {
                             static char output_buffer[8192];
                             strncpy(output_buffer, success->output_json.c_str(), sizeof(output_buffer) - 1);
                             ImGui::InputTextMultiline("##Output", output_buffer, sizeof(output_buffer), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);
                        } else if (const auto* error = std::get_if<MITSU_Domoe::ErrorResult>(&(*result))) {
                            static char error_buffer[2048];
                            strncpy(error_buffer, error->error_message.c_str(), sizeof(error_buffer) - 1);
                            ImGui::InputTextMultiline("##Error", error_buffer, sizeof(error_buffer), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);
                        }
                    } else {
                        ImGui::Text("Result not yet available.");
                    }
                } else {
                     ImGui::Text("Select a command from the history to inspect its result.");
                }
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    processor.stop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
