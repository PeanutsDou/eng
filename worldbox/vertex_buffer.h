#pragma once
#include <cstdint>
#include <vector>

// ===== VertexBufferWriter =====
// A "canvas" that collects vertices and indices.
// Call add_vertex() to place points, add_triangle() to connect them.
// Does NOT know or care what shape is being built.

struct VertexBufferWriter {
    std::vector<float> vertices;       // flat float array: x0,y0, x1,y1, ...
    std::vector<uint32_t> indices;

    // How many floats per vertex (currently 2: x, y)
    static constexpr int stride = 2;

    // Current vertex index (starts at 0, increments with each add_vertex)
    int current_vertex() const {
        return (int)vertices.size() / stride;
    }

    // Place a new vertex at (x, y). Returns its index.
    int add_vertex(float x, float y) {
        vertices.push_back(x);
        vertices.push_back(y);
        return current_vertex() - 1;
    }

    // Connect three vertices into a triangle.
    void add_triangle(int a, int b, int c) {
        indices.push_back((uint32_t)a);
        indices.push_back((uint32_t)b);
        indices.push_back((uint32_t)c);
    }

    // How many vertices have been placed so far.
    int vertex_count() const {
        return current_vertex();
    }

    // How many indices (triangles * 3) have been added so far.
    int index_count() const {
        return (int)indices.size();
    }

    // Clear everything.
    void clear() {
        vertices.clear();
        indices.clear();
    }
};
