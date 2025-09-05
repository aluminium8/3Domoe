#pragma once

#include "3D_objects.hpp"
#include "Shader.hpp"

#include <glad/glad.h>

namespace MITSU_Domoe {

class ModelRenderer {
public:
    ModelRenderer(const Polygon_mesh& mesh);
    ~ModelRenderer();

    void draw(Shader& shader);

private:
    GLuint VAO, VBO, EBO;
    GLsizei index_count;
};

}
