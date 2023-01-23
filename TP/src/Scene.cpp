#include "Scene.h"

#include <TypedBuffer.h>

#include <shader_structs.h>

namespace OM3D {

Scene::Scene() {
}

void Scene::add_object(SceneObject obj, int mat) {
    //trier les objets en 2 vecteurs différents en comparant les matériaux
    if (mat == 1)
        _trees.emplace_back(std::move(obj));
    else
        _rocks.emplace_back(std::move(obj));
}

void Scene::add_object(PointLight obj) {
    _point_lights.emplace_back(std::move(obj));
}

void Scene::render(const Camera& camera) const {
    // Fill and bind frame data buffer
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = camera.view_proj_matrix();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].sun_color = glm::vec3(1.0f, 1.0f, 1.0f);
        mapping[0].sun_dir = glm::normalize(_sun_direction);
    }
    buffer.bind(BufferUsage::Uniform, 0);

    // Fill and bind lights buffer
    TypedBuffer<shader::PointLight> light_buffer(nullptr, std::max(_point_lights.size(), size_t(1)));
    {
        auto mapping = light_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _point_lights.size(); ++i) {
            const auto& light = _point_lights[i];
            mapping[i] = {
                light.position(),
                light.radius(),
                light.color(),
                0.0f
            };
        }
    }
    light_buffer.bind(BufferUsage::Storage, 1);


    //instanced rendering
    TypedBuffer<glm::mat4> obj_buffer(nullptr, std::max(_trees.size(), size_t(1)));
    {
        auto mapping = obj_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _trees.size(); ++i) {
            const auto& obj = _trees[i];
            mapping[i] = obj.transform();
        }
    }
    obj_buffer.bind(BufferUsage::Storage, 2);

    _trees[0].render(_trees.size());

    //TypedBuffer<glm::mat4> obj_buffer(nullptr, std::max(_trees.size(), size_t(1)));
    {
        auto mapping = obj_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _rocks.size(); ++i) {
            const auto& obj = _rocks[i];
            mapping[i] = obj.transform();
        }
    }
    obj_buffer.bind(BufferUsage::Storage, 2);

    _rocks[0].render(_rocks.size());

    // Render every object
    //for(const SceneObject& obj : _objects) {
    //    obj.render();
    //}

}

}
