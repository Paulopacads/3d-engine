#ifndef STATICMESH_H
#define STATICMESH_H

#include <graphics.h>
#include <TypedBuffer.h>
#include <Vertex.h>

#include <vector>

#include "Camera.h"
namespace OM3D {

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
};

class StaticMesh : NonCopyable {

    public:
        StaticMesh() = default;
        StaticMesh(StaticMesh&&) = default;
        StaticMesh& operator=(StaticMesh&&) = default;

        StaticMesh(const MeshData& data);

        void draw(unsigned int elts = 1) const;

        bool is_forward(const glm::vec3& normal, const glm::vec3& origin) const;
        bool frustum_collide(const Frustum& frustum, const glm::vec3& origin) const;

    private:
        glm::vec3 _bounding_center;
        float _bounding_radius;
        TypedBuffer<Vertex> _vertex_buffer;
        TypedBuffer<u32> _index_buffer;
};

}

#endif // STATICMESH_H
