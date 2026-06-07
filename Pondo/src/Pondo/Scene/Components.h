#pragma once

#include "../Core.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)

    struct PONDO_API TagComponent {
        std::string Tag = "Entity";
        TagComponent() = default;
        TagComponent(const std::string& tag) : Tag(tag) {}
    };

    struct PONDO_API TransformComponent {
        glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

        TransformComponent() = default;

        glm::mat4 GetTransform() const {
            glm::mat4 t = glm::translate(glm::mat4(1.0f), Position);
            glm::mat4 r = glm::eulerAngleXYZ(
                glm::radians(Rotation.x),
                glm::radians(Rotation.y),
                glm::radians(Rotation.z));
            glm::mat4 s = glm::scale(glm::mat4(1.0f), Scale);
            return t * r * s;
        }
    };

    // Mesh + material live on the entity so the scene owns everything
    struct PONDO_API MeshComponent {
        std::shared_ptr<Mesh> MeshData;
        MeshComponent() = default;
        MeshComponent(std::shared_ptr<Mesh> m) : MeshData(std::move(m)) {}
    };

    struct PONDO_API MaterialComponent {
        std::shared_ptr<Material> Mat;
        MaterialComponent() = default;
        MaterialComponent(std::shared_ptr<Material> m) : Mat(std::move(m)) {}
    };

#pragma warning(pop)

}
