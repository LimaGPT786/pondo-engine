#include "Entity.h"

namespace Pondo {

    uint32_t Entity::s_NextID = 1;

    Entity::Entity(const std::string& name)
        : m_ID(s_NextID++), m_Tag(name)
    {
    }

    uint32_t Entity::GetID() const { return m_ID; }

    const std::string& Entity::GetTag() const { return m_Tag.Tag; }

    void Entity::SetTag(const std::string& tag) { m_Tag.Tag = tag; }

    TransformComponent& Entity::GetTransform() { return m_Transform; }

    const TransformComponent& Entity::GetTransform() const { return m_Transform; }

    MeshComponent* Entity::GetMesh()
    {
        return m_HasMesh ? &m_Mesh : nullptr;
    }

    const MeshComponent* Entity::GetMesh() const
    {
        return m_HasMesh ? &m_Mesh : nullptr;
    }

    MaterialComponent* Entity::GetMaterial()
    {
        return m_HasMaterial ? &m_Material : nullptr;
    }

    const MaterialComponent* Entity::GetMaterial() const
    {
        return m_HasMaterial ? &m_Material : nullptr;
    }

    void Entity::SetMesh(std::shared_ptr<Mesh> mesh)
    {
        m_Mesh.MeshData = std::move(mesh);
        m_HasMesh = true;
    }

    void Entity::SetMaterial(std::shared_ptr<Material> mat)
    {
        m_Material.Mat = std::move(mat);
        m_HasMaterial = true;
    }

}