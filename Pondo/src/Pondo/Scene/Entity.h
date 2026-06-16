#pragma once

#include "../Core.h"
#include "Components.h"
#include <string>
#include <memory>
#include <entt.hpp>

// Forward-declare the registry so Entity.h doesn't pull in all of entt
// in every translation unit that includes it.

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)

    // Entity is a lightweight handle: an entt entity ID + a pointer back to
    // the registry that owns it.  All component storage lives in the registry;
    // Entity methods are thin wrappers around registry get/emplace/remove.
    class PONDO_API Entity {
    public:

        Entity() = default;
        Entity(entt::entity id, entt::registry* reg);

        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;
        Entity(Entity&&) = default;
        Entity& operator=(Entity&&) = default;

        uint32_t           GetID()  const;
        const std::string& GetTag() const;
        void               SetTag(const std::string& tag);

        TransformComponent&       GetTransform();
        const TransformComponent& GetTransform() const;

        MeshComponent*            GetMesh();
        const MeshComponent*      GetMesh()     const;
        MaterialComponent*        GetMaterial();
        const MaterialComponent*  GetMaterial() const;
        LightComponent*           GetLight();
        const LightComponent*     GetLight()    const;

        void SetMesh    (std::shared_ptr<Mesh>     mesh);
        void SetMaterial(std::shared_ptr<Material> mat);
        void AddLight   (LightType type = LightType::Point);
        void RemoveLight();

        bool HasLight()    const;
        bool HasMesh()     const;
        bool HasMaterial() const;
        // Raw entt handle — needed by Scene internals only.
        entt::entity Handle() const { return m_Handle; }
        bool         Valid()  const { return m_Registry != nullptr; }

        ScriptComponent* GetScript();
        const ScriptComponent* GetScript() const;
        void                   SetScript(const std::string& path);
        void                   RemoveScript();
        bool                   HasScript() const;

        // Group
        GroupRootComponent* GetGroupRoot();
        const GroupRootComponent* GetGroupRoot() const;
        void                      MakeGroupRoot(const std::string& name = "Group");
        bool                      IsGroupRoot() const;

        GroupComponent* GetGroup();
        const GroupComponent* GetGroup() const;
        void                      SetGroup(uint32_t parentID, bool negate = false);
        void                      RemoveFromGroup();
        bool                      InGroup() const;
        bool                      IsNegate() const;

    private:
        entt::entity    m_Handle   = entt::entity(0xFFFFFFFF);
        entt::registry* m_Registry = nullptr;
    };

#pragma warning(pop)

} // namespace Pondo
