#include "Scene.h"

#include <TypedBuffer.h>

#include <shader_structs.h>

#include <algorithm>
#include <unordered_map>

namespace OM3D {

Scene::Scene() {
}

void Scene::add_object(SceneObject obj) {
    _objects.emplace_back(std::move(obj));
}

void Scene::add_object(PointLight obj) {
    _point_lights.emplace_back(std::move(obj));
}

std::shared_ptr<TypedBuffer<shader::FrameData>> Scene::frame_data_buffer(const Camera& camera) const {
    std::shared_ptr<TypedBuffer<shader::FrameData>> buffer =
        std::make_shared<TypedBuffer<shader::FrameData>>(nullptr, 1);
        
    auto mapping = buffer->map(AccessType::WriteOnly);
    mapping[0].camera.view_proj = camera.view_proj_matrix();
    mapping[0].point_light_count = u32(_point_lights.size());
    mapping[0].sun_color = glm::vec3(1.0f, 1.0f, 1.0f);
    mapping[0].sun_dir = glm::normalize(_sun_direction);
    mapping[0].sun_view_proj = sun_view_proj(camera);
        
    return buffer;
}

std::shared_ptr<TypedBuffer<shader::PointLight>> Scene::point_light_buffer() const {
    std::shared_ptr<TypedBuffer<shader::PointLight>> buffer =
        std::make_shared<TypedBuffer<shader::PointLight>>(nullptr, std::max(_point_lights.size(), size_t(1)));
    
    auto mapping = buffer->map(AccessType::WriteOnly);
    for(size_t i = 0; i != _point_lights.size(); ++i) {
        const auto& light = _point_lights[i];
        mapping[i] = {
            light.position(),
            light.radius(),
            light.color(),
            0.0f
        };
    }

    return buffer;
}

glm::mat4 Scene::sun_view_proj(const Camera& camera) const {
    glm::mat4 reverse = glm::mat4(1.0);
    reverse[2][2] = -1.0f;
    reverse[3][2] = -1.0f;
    glm::mat4 proj = reverse * glm::orthoZO<float>(-128, 128, -128, 128, -1024, 1024);
    glm::vec3 up = glm::cross(glm::cross(_sun_direction, glm::vec3(0.0f, 1.0f, 0.0f)), _sun_direction);
    glm::mat4 view = glm::lookAt(camera.position() + _sun_direction, camera.position(), up);
    return proj * view;
}

void Scene::render(const Camera& camera) const {
    // Fill and bind frame data buffer
    const std::shared_ptr<TypedBuffer<shader::FrameData>> frame_buffer = frame_data_buffer(camera);
    frame_buffer->bind(BufferUsage::Uniform, 0);

    // Fill and bind lights buffer
    const std::shared_ptr<TypedBuffer<shader::PointLight>> light_buffer = point_light_buffer();
    light_buffer->bind(BufferUsage::Storage, 1);

    std::unordered_map<Material*, std::vector<const SceneObject *>> objects_by_material =
        std::unordered_map<Material*, std::vector<const SceneObject*>>();

    for (const SceneObject &obj : _objects) {
        // FRUSTUM CULLING ?

        Material* material_ptr = obj.material().get();
        objects_by_material[material_ptr].push_back(&obj);
    }

    for (const auto& [material, objects] : objects_by_material) {
        for (const SceneObject *obj : objects) {
            material->set_uniform(HASH("model"), obj->transform());
            material->bind();
            obj->mesh()->draw();
        }
    }
}

void Scene::render_shadowmap(const Camera& camera) const {
    std::unordered_map<Material*, std::vector<const SceneObject *>> objects_by_material =
        std::unordered_map<Material*, std::vector<const SceneObject*>>();

    for (const SceneObject &obj : _objects) {
        // FRUSTUM CULLING ?

        Material* material_ptr = obj.material().get();
        objects_by_material[material_ptr].push_back(&obj);
    }

    for (const auto& [material, objects] : objects_by_material) {
        const std::shared_ptr<TypedBuffer<shader::Model>> transform =
            std::make_shared<TypedBuffer<shader::Model>>(nullptr, std::max(objects.size(), size_t(1)));

        auto mapping = transform->map(AccessType::WriteOnly);
        for (size_t i = 0; i != objects.size(); ++i) {
            mapping[i].transform = objects[i]->transform();
        }
        
        transform->bind(BufferUsage::Storage, 2);

        for (const SceneObject *obj : objects) {
            obj->mesh()->draw();
        }
    }
}

}
