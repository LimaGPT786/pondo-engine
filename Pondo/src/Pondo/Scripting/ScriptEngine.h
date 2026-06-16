#pragma once

#include "Pondo/Core.h"
#include <string>
#include <memory>

// Forward-declare sol/lua state so we don't blow up every TU that includes this
struct lua_State;
namespace sol { class state; }

namespace Pondo {

    class Scene;
    class Entity;

    class PONDO_API ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();

        // Called by SceneLayer on play/stop
        static void OnPlayStart(Scene* scene);
        static void OnUpdate(Scene* scene, float dt);
        static void OnPlayStop();

        static bool IsInitialized();

    private:
        static void RegisterBindings();
        static void RunEntityScript(Entity* entity, Scene* scene);
    };

} // namespace Pondo