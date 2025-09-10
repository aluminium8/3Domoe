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
    void process_mesh_results();

    ShaderManager shader_manager;
    std::map<std::pair<uint64_t, std::string>, MeshRenderState> mesh_render_states;
};

}
