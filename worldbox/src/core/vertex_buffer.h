#pragma once
#include <cstdint>
#include <vector>

// CPU-side vertex data collector.
// Push floats one by one (or via push_* helpers), add triangle indices.
// Does NOT know or care what shape is being built — purely a data bucket.
//
// Usage:
//   VertexBufferWriter geom;
//   geom.push_float(x); geom.push_float(y);    // position
//   geom.push_float(r); geom.push_float(g); geom.push_float(b); // color
//   int v0 = geom.commit_vertex(5);  // 5 floats per vertex
//   ...
//   geom.add_triangle(v0, v1, v2);
struct VertexBufferWriter {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    // Push raw floats (caller manages the layout).
    void push_float(float v) { vertices.push_back(v); }

    // Convenience: push vec2/vec3/vec4 at once.
    void push_vec2(float x, float y) {
        vertices.push_back(x);
        vertices.push_back(y);
    }
    void push_vec3(float x, float y, float z) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
    }
    void push_vec4(float x, float y, float z, float w) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(w);
    }

    // Call after pushing one vertex worth of floats.
    // `floats_per_vertex` must match the VertexLayout stride.
    // Returns the vertex index (for add_triangle).
    int commit_vertex(int floats_per_vertex) {
        int idx = (int)vertices.size() / floats_per_vertex - 1;
        return idx;
    }

    // How many vertices recorded so far.
    int vertex_count(int floats_per_vertex) const {
        return (int)vertices.size() / floats_per_vertex;
    }

    // Connect three vertices into a triangle.
    void add_triangle(int a, int b, int c) {
        indices.push_back((uint32_t)a);
        indices.push_back((uint32_t)b);
        indices.push_back((uint32_t)c);
    }

    int index_count() const {
        return (int)indices.size();
    }

    void clear() {
        vertices.clear();
        indices.clear();
    }
};
