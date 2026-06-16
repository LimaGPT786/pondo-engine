#pragma once

#include "../Core.h"
#include "Entity.h"
#include <vector>
#include <memory>
#include <string>
#include <entt.hpp>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)

    class PONDO_API Scene {
    public:
        Scene(const std::string& name = "Untitled Scene");
        ~Scene();

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = default;
        Scene& operator=(Scene&&) = default;

        Entity* CreateEntity(const std::string& name = "Entity");
        void    DestroyEntity(uint32_t id);

        Entity* FindEntity(uint32_t id);
        Entity* FindEntityByName(const std::string& name);

        // Returns stable pointers — valid until the entity is destroyed or
        // the scene is cleared.
        const std::vector<std::unique_ptr<Entity>>& GetEntities() const;

        const std::string& GetName() const;
        void SetName(const std::string& name);
        void Clear();

        enum class PlayState { Edit, Playing, Paused };

        void        Play();
        void        Pause();
        void        Stop();
        PlayState   GetPlayState() const { return m_PlayState; }

    private:
        std::string                          m_Name;
        std::unique_ptr<entt::registry>      m_Registry;
        std::vector<std::unique_ptr<Entity>> m_Entities; // handle wrappers (ordered)

        PlayState   m_PlayState = PlayState::Edit;

        // Snapshot for restoring scene on Stop
        struct EntitySnapshot
        {
            std::string              Tag;
            Pondo::TransformComponent Transform;
            bool                     HasMesh;
            bool                     HasMaterial;
            bool                     HasLight;
            bool                     HasScript;
            glm::vec4                MaterialColor;
            Pondo::LightComponent    Light;
            Pondo::ScriptComponent   Script;
        };
        std::vector<EntitySnapshot> m_SceneSnapshot;

        void TakeSnapshot();
        void RestoreSnapshot();
    };

#pragma warning(pop)

} // namespace Pondo
