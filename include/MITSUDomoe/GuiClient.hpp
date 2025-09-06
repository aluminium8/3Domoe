#pragma once

#include "BaseClient.hpp"
#include "Shader.hpp"
#include "Renderer.hpp"
#include "3D_objects.hpp"
#include <map>
#include <memory>
#include <string>


namespace MITSU_Domoe {

struct MeshRenderState
{
    std::unique_ptr<Renderer> renderer;
    bool is_visible = true;
    std::string selected_shader_name = "edge";
};


class GuiClient : public BaseClient
{
public:
    GuiClient();
    ~GuiClient() = default;

    void run() override;

private:
    void process_stl_results();

    ShaderManager shader_manager;
    std::map<uint64_t, MeshRenderState> mesh_render_states;
};

}
