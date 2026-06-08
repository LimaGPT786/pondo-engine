#pragma once

#include "../Core.h"
#include "Entity.h"
#include <vector>
#include <memory>
#include <string>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)
    class PONDO_API Scene {
    public:
        Scene(const std::string& name = "Untitled Scene");
        ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = default;
        Scene& operator=(Scene&&) = default;

        Entity* CreateEntity(const std::string& name = "Entity");
        void    DestroyEntity(uint32_t id);

        Entity* FindEntity(uint32_t id);
        Entity* FindEntityByName(const std::string& name);

        const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return m_Entities; }

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }
        void Clear() { m_Entities.clear(); }
    private:
        std::string m_Name;
        std::vector<std::unique_ptr<Entity>> m_Entities;
    };
#pragma warning(pop)

}