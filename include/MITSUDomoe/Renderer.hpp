#pragma once

#include <glad/glad.h>
#include <vector>

#include "MITSUDomoe/3D_objects.hpp"
#include "MITSUDomoe/Shader.hpp"

namespace MITSU_Domoe
{

    class Renderer
    {
    public:
        unsigned int VAO, VBO, EBO;

        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        Renderer(const Polygon_mesh &mesh)
        {
            setup_mesh(mesh);
        }

        ~Renderer()
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        void draw(const Shader &shader) const
        {
            shader.use();

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

    private:
        void setup_mesh(const Polygon_mesh &mesh)
        {
            // Convert vertices from double to float
            vertices.reserve(mesh.V.rows() * 3);
            for (int i = 0; i < mesh.V.rows(); ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    vertices.push_back(static_cast<float>(mesh.V(i, j)));
                }
            }

            // Convert faces to a flat list of indices
            indices.reserve(mesh.F.rows() * 3);
            for (int i = 0; i < mesh.F.rows(); ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    indices.push_back(mesh.F(i, j));
                }
            }

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            // vertex positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

            glBindVertexArray(0);
        }
    };

}
