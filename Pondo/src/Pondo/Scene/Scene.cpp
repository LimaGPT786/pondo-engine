#include "Scene.h"
#include "Components.h"

#pragma warning(push)
#pragma warning(disable: 4267 4244 5054)
#include <entt.hpp>
#pragma warning(pop)

#include <algorithm>

namespace Pondo {

    Scene::Scene(const std::string& name)
        : m_Name(name), m_Registry(std::make_unique<entt::registry>())
    {
    }

    // Needs to be defined here because entt::registry is an incomplete type
    // in the header.
    Scene::~Scene() = default;

    // ---- Entity management ---------------------------------------------

    Entity* Scene::CreateEntity(const std::string& name)
    {
        entt::entity handle = m_Registry->create();

        // Every entity always has a tag and a transform.
        m_Registry->emplace<TagComponent>(handle, name);
        m_Registry->emplace<TransformComponent>(handle);

        auto entity = std::make_unique<Entity>(handle, m_Registry.get());
        m_Entities.push_back(std::move(entity));
        return m_Entities.back().get();
    }

    void Scene::DestroyEntity(uint32_t id)
    {
        auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
            [id](const std::unique_ptr<Entity>& e)
            { return e->GetID() == id; });

        if (it != m_Entities.end())
        {
            m_Registry->destroy((*it)->Handle());
            m_Entities.erase(it);
        }
    }

    Entity* Scene::FindEntity(uint32_t id)
    {
        auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
            [id](const std::unique_ptr<Entity>& e)
            { return e->GetID() == id; });
        return (it != m_Entities.end()) ? it->get() : nullptr;
    }

    Entity* Scene::FindEntityByName(const std::string& name)
    {
        auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
            [&name](const std::unique_ptr<Entity>& e)
            { return e->GetTag() == name; });
        return (it != m_Entities.end()) ? it->get() : nullptr;
    }

    const std::vector<std::unique_ptr<Entity>>& Scene::GetEntities() const
    {
        return m_Entities;
    }

    const std::string& Scene::GetName() const { return m_Name; }
    void Scene::SetName(const std::string& name) { m_Name = name; }

    void Scene::Clear()
    {
        m_Registry->clear();
        m_Entities.clear();
    }

} // namespace Pondo
