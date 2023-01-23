#include "StaticMesh.h"

#include <glad/glad.h>

namespace OM3D {

StaticMesh::StaticMesh(const MeshData& data) :
    _vertex_buffer(data.vertices),
    _index_buffer(data.indices) {
    glm::vec3 max(data.vertices.at(0).position);
    glm::vec3 min(data.vertices.at(0).position);

    for (Vertex v : data.vertices)
    {
        if (v.position.x < min.x)
            min.x = v.position.x;
        else if (v.position.x > max.x)
            max.x = v.position.x;
        if (v.position.y < min.y)
            min.y = v.position.y;
        else if (v.position.y > max.y)
            max.y = v.position.y;
        if (v.position.z < min.z)
            min.z = v.position.z;
        else if (v.position.z > max.z)
            max.z = v.position.z;
    }
    
    _bounding_center = min + (glm::vec3((max.x - min.x) / 2, (max.y - min.y) / 2, (max.z - min.z) / 2));
    _bounding_radius = (max - _bounding_center).length();
}

void StaticMesh::setup() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    
}

void StaticMesh::draw() const {
    setup();
    glDrawElements(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr);
}

void StaticMesh::draw_instanced(u32 count) const {
    setup();
    glDrawElementsInstanced(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr, count);
}

}
