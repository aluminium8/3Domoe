#pragma once

#include "MITSUDomoe/3D_objects.hpp"
#include "MITSUDomoe/Shader.hpp"
#include <glad/glad.h>

namespace MITSU_Domoe {

class RenderableMesh {
public:
    RenderableMesh(const Polygon_mesh& mesh);
    ~RenderableMesh();

    void draw(Shader& shader);

private:
    GLuint VAO, VBO, EBO;
    GLsizei index_count;
};

}
