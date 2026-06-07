#pragma once

#include "../Core.h"
#include "Components.h"
#include <string>
#include <memory>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)

    class PONDO_API Entity {
    public:
        Entity() = default;
        Entity(const std::string& name)
            : m_ID(s_NextID++), m_Tag(name) {
        }

        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;
        Entity(Entity&&) = default;
        Entity& operator=(Entity&&) = default;

        uint32_t           GetID()  const { return m_ID; }
        const std::string& GetTag() const { return m_Tag.Tag; }
        void               SetTag(const std::string& tag) { m_Tag.Tag = tag; }

        TransformComponent& GetTransform() { return m_Transform; }
        const TransformComponent& GetTransform() const { return m_Transform; }

        // Returns nullptr if no mesh has been set
        MeshComponent* GetMesh() { return m_HasMesh ? &m_Mesh : nullptr; }
        MaterialComponent* GetMaterial() { return m_HasMaterial ? &m_Material : nullptr; }

        const MeshComponent* GetMesh()     const { return m_HasMesh ? &m_Mesh : nullptr; }
        const MaterialComponent* GetMaterial() const { return m_HasMaterial ? &m_Material : nullptr; }

        void SetMesh(std::shared_ptr<Mesh> mesh) {
            m_Mesh.MeshData = std::move(mesh);
            m_HasMesh = true;
        }

        void SetMaterial(std::shared_ptr<Material> mat) {
            m_Material.Mat = std::move(mat);
            m_HasMaterial = true;
        }

    private:
        uint32_t           m_ID = 0;
        TagComponent       m_Tag;
        TransformComponent m_Transform;

        MeshComponent     m_Mesh;
        MaterialComponent m_Material;
        bool              m_HasMesh = false;
        bool              m_HasMaterial = false;

        static uint32_t s_NextID;
    };

#pragma warning(pop)

}