#include "Scene.h"
#include <algorithm>

namespace Pondo {

	Scene::Scene(const std::string& name)
		: m_Name(name)
	{
	}

	Entity* Scene::CreateEntity(const std::string& name)
	{
		m_Entities.push_back(std::make_unique<Entity>(name));
		return m_Entities.back().get();
	}

	void Scene::DestroyEntity(uint32_t id)
	{
		auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
			[id](const std::unique_ptr<Entity>& e) { return e->GetID() == id; });

		if (it != m_Entities.end())
			m_Entities.erase(it);
	}

	Entity* Scene::FindEntity(uint32_t id)
	{
		auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
			[id](const std::unique_ptr<Entity>& e) { return e->GetID() == id; });

		return (it != m_Entities.end()) ? it->get() : nullptr;
	}

	Entity* Scene::FindEntityByName(const std::string& name)
	{
		auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
			[&name](const std::unique_ptr<Entity>& e) { return e->GetTag() == name; });

		return (it != m_Entities.end()) ? it->get() : nullptr;
	}

}