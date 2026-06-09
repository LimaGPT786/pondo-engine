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
        Entity(const std::string& name);

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
        void AddLight   (LightType type = LightType::Point); // attaches a LightComponent
        void RemoveLight();

        bool HasLight() const { return m_HasLight; }

    private:
        uint32_t           m_ID = 0;
        TagComponent       m_Tag;
        TransformComponent m_Transform;

        MeshComponent      m_Mesh;
        MaterialComponent  m_Material;
        LightComponent     m_Light;

        bool m_HasMesh     = false;
        bool m_HasMaterial = false;
        bool m_HasLight    = false;

        static uint32_t s_NextID;
    };

#pragma warning(pop)

} // namespace Pondo
