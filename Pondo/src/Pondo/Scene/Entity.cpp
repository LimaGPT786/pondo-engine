#include "Entity.h"

// Pull in entt only in this translation unit.
#pragma warning(push)
#pragma warning(disable: 4267 4244 5054)
#include <entt.hpp>
#pragma warning(pop)

namespace Pondo {

    Entity::Entity(entt::entity id, entt::registry* reg)
        : m_Handle(id), m_Registry(reg)
    {
    }

    // ---- Identity -------------------------------------------------------

    uint32_t Entity::GetID() const
    {
        // entt entity values encode both index and generation; the lower 20 bits
        // give a stable, dense ID suitable for the editor's uint32_t IDs.
        return static_cast<uint32_t>(m_Handle);
    }

    const std::string& Entity::GetTag() const
    {
        return m_Registry->get<TagComponent>(m_Handle).Tag;
    }

    void Entity::SetTag(const std::string& tag)
    {
        m_Registry->get<TagComponent>(m_Handle).Tag = tag;
    }

    // ---- Transform (always present) ------------------------------------

    TransformComponent& Entity::GetTransform()
    {
        return m_Registry->get<TransformComponent>(m_Handle);
    }

    const TransformComponent& Entity::GetTransform() const
    {
        return m_Registry->get<TransformComponent>(m_Handle);
    }

    // ---- Mesh ----------------------------------------------------------

    MeshComponent* Entity::GetMesh()
    {
        return m_Registry->try_get<MeshComponent>(m_Handle);
    }

    const MeshComponent* Entity::GetMesh() const
    {
        return m_Registry->try_get<MeshComponent>(m_Handle);
    }

    bool Entity::HasMesh() const
    {
        return m_Registry->all_of<MeshComponent>(m_Handle);
    }

    void Entity::SetMesh(std::shared_ptr<Mesh> mesh)
    {
        auto& mc = m_Registry->get_or_emplace<MeshComponent>(m_Handle);
        mc.MeshData = std::move(mesh);
    }

    // ---- Material ------------------------------------------------------

    MaterialComponent* Entity::GetMaterial()
    {
        return m_Registry->try_get<MaterialComponent>(m_Handle);
    }

    const MaterialComponent* Entity::GetMaterial() const
    {
        return m_Registry->try_get<MaterialComponent>(m_Handle);
    }

    bool Entity::HasMaterial() const
    {
        return m_Registry->all_of<MaterialComponent>(m_Handle);
    }

    void Entity::SetMaterial(std::shared_ptr<Material> mat)
    {
        auto& mc = m_Registry->get_or_emplace<MaterialComponent>(m_Handle);
        mc.Mat = std::move(mat);
    }

    // ---- Light ---------------------------------------------------------

    LightComponent* Entity::GetLight()
    {
        return m_Registry->try_get<LightComponent>(m_Handle);
    }

    const LightComponent* Entity::GetLight() const
    {
        return m_Registry->try_get<LightComponent>(m_Handle);
    }

    bool Entity::HasLight() const
    {
        return m_Registry->all_of<LightComponent>(m_Handle);
    }

    void Entity::AddLight(LightType type)
    {
        auto& lc  = m_Registry->get_or_emplace<LightComponent>(m_Handle);
        lc        = LightComponent{};
        lc.Type   = type;
    }

    void Entity::RemoveLight()
    {
        m_Registry->remove<LightComponent>(m_Handle);
    }

} // namespace Pondo
