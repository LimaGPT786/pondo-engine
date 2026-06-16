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
        glm::vec3 Scale    = { 1.0f, 1.0f, 1.0f };

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

    // ---- Light component (attach to any entity) -------------------
    // Works like Roblox's PointLight / SpotLight / SurfaceLight parts.
    // Directional acts as the global sun when SceneLayer collects lights.

    enum class LightType : uint8_t
    {
        Directional = 0,
        Point       = 1,
        Spot        = 2,
    };

    struct PONDO_API LightComponent
    {
        LightType  Type      = LightType::Point;
        bool       Enabled   = true;

        glm::vec3  Color     = { 1.0f, 1.0f, 1.0f };
        float      Intensity = 1.0f;

        // Directional — direction the light faces (normalised world space).
        // For point/spot the position is taken from TransformComponent.
        glm::vec3  Direction = { -0.4f, -1.0f, -0.3f };

        // Point & Spot attenuation
        float      Range     = 10.0f;
        float      Constant  = 1.0f;
        float      Linear    = 0.09f;
        float      Quadratic = 0.032f;

        // Spot only
        float      InnerCutoffDeg = 12.5f;
        float      OuterCutoffDeg = 17.5f;

        LightComponent() = default;
    };

    struct PONDO_API ScriptComponent
    {
        std::string ScriptPath;   // path to .lua file on disk
        bool        HasError = false;
        std::string ErrorMsg;

        ScriptComponent() = default;
        ScriptComponent(const std::string& path) : ScriptPath(path) {}
    };

    struct PONDO_API GroupRootComponent
    {
        std::string Name = "Group";
        bool        Unioned = false; // true = CSG baked (phase 2)
        GroupRootComponent() = default;
        GroupRootComponent(const std::string& n) : Name(n) {}
    };

    struct PONDO_API GroupComponent
    {
        uint32_t ParentID = 0;      // ID of the GroupRoot entity
        bool     IsNegate = false;  // true = subtracts in CSG union
        GroupComponent() = default;
        GroupComponent(uint32_t pid, bool negate = false)
            : ParentID(pid), IsNegate(negate) {
        }
    };

    // Marks a standalone (pre-group) entity as a boolean subtractor.
    // Consumed by CreateGroup: transferred into GroupComponent::IsNegate.
    struct PONDO_API PendingNegateComponent
    {
        PendingNegateComponent() = default;
    };

#pragma warning(pop)

} // namespace Pondo
