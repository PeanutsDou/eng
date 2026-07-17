#include "mesh.h"

void Mesh::upload(const std::vector<float>& vertices,
                   const std::vector<uint32_t>& indices,
                   int stride) {
    // Free any old GPU resources first
    cleanup();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Attribute 0: position (first 2 floats of each vertex)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          stride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Upload index data (if any)
    if (!indices.empty()) {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(uint32_t),
                     indices.data(),
                     GL_STATIC_DRAW);
        index_count = (int)indices.size();
    } else {
        index_count = 0;
    }

    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    if (index_count > 0) {
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
    } else {
        // Fallback: draw all vertices as triangles (crude, caller should use indices)
        // This path is intentionally not used — indices should always be provided.
    }
}

void Mesh::cleanup() {
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    index_count = 0;
}
