#include "core/mesh.h"

void Mesh::upload(const float* vertices, int vertex_count,
                   const uint32_t* indices, int idx_count,
                   const VertexLayout& layout) {
    // Free any old GPU resources first
    cleanup();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertex_count * layout.stride_bytes,
                 vertices,
                 GL_STATIC_DRAW);

    // Configure every attribute from the layout
    for (const auto& attr : layout.attrs) {
        glVertexAttribPointer(
            attr.location,
            attr.count,
            attr.type,
            GL_FALSE,
            layout.stride_bytes,
            (void*)attr.offset
        );
        glEnableVertexAttribArray(attr.location);
    }

    // Upload index data (if any)
    if (idx_count > 0 && indices != nullptr) {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     idx_count * sizeof(uint32_t),
                     indices,
                     GL_STATIC_DRAW);
        index_count = idx_count;
    } else {
        index_count = 0;
    }

    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    if (index_count > 0) {
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
    }
}

void Mesh::cleanup() {
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    index_count = 0;
}
