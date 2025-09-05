#include "MITSUDomoe/ModelRenderer.hpp"
#include <vector>

namespace MITSU_Domoe {

ModelRenderer::ModelRenderer(const Polygon_mesh& mesh) {
    index_count = mesh.F.rows() * mesh.F.cols();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Buffer vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.V.size() * sizeof(double), mesh.V.data(), GL_STATIC_DRAW);

    // Buffer element/index data
    Eigen::MatrixXi F_transposed = mesh.F;
    F_transposed.transposeInPlace(); // igl F is #F x 3, opengl wants 3 x #F then flatten
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, F_transposed.size() * sizeof(int), F_transposed.data(), GL_STATIC_DRAW);

    // Set the vertex attribute pointers
    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), (void*)0);

    glBindVertexArray(0);
}

ModelRenderer::~ModelRenderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void ModelRenderer::draw(Shader& shader) {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

}
