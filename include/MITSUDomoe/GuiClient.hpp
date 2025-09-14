#pragma once

#include "BaseClient.hpp"
#include "Shader.hpp"
#include "Renderer.hpp"
#include "3D_objects.hpp"
#include <map>
#include <memory>
#include <string>
#include <filesystem>
#include <utility>

struct yyjson_val;

namespace MITSU_Domoe {

struct MeshRenderState
{
    std::unique_ptr<Renderer> renderer;
    bool is_visible = true;
    std::string selected_shader_name = "barycentric_wireframe";

    // Camera state
    Eigen::Vector3f camera_target;
    float distance;
    float yaw = -90.0f;
    float pitch = 0.0f;
    float near_clip;
    float far_clip;
};


class GuiClient : public BaseClient
{
public:
    GuiClient(const std::filesystem::path& log_path);
    ~GuiClient() = default;

    void run() override;

private:
    void render_json_tree_node(yyjson_val *node, const std::string &current_path, uint64_t result_id);
    void process_mesh_results();
    void handle_load(const std::string& path_str);
    void handle_trace(const std::string& path_str);
    void handle_trace_history(const std::string& path_str);

    ShaderManager shader_manager;
    std::map<std::pair<uint64_t, std::string>, MeshRenderState> mesh_render_states;

    // UI State for Log Loader
    char log_path_buffer[256] = {0};
    std::string loaded_log_content;
};

}
