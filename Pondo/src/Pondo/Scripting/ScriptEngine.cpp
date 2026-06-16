#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include "ScriptEngine.h"

#pragma warning(push)
#pragma warning(disable: 4996 4244 4267 4312)
#include <sol/sol.hpp>
#pragma warning(pop)

#include "Pondo/Scene/Scene.h"
#include "Pondo/Scene/Entity.h"
#include "Pondo/Scene/Components.h"
#include "Pondo/Input.h"
#include "Pondo/Log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Pondo {

    // -------------------------------------------------------
    //  Static state
    // -------------------------------------------------------

    static std::unique_ptr<sol::state> s_Lua;

    bool ScriptEngine::IsInitialized() { return s_Lua != nullptr; }

    static Pondo::Scene* s_ActiveScene = nullptr;
    static std::unordered_map<uint32_t, sol::environment> s_Environments;

    // -------------------------------------------------------
    //  Init / Shutdown
    // -------------------------------------------------------

    void ScriptEngine::Init()
    {
        s_Lua = std::make_unique<sol::state>();
        s_Lua->open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table,
            sol::lib::io);

        RegisterBindings();
        PD_CORE_INFO("ScriptEngine initialised (Lua 5.4 + sol2)");
    }

    void ScriptEngine::Shutdown()
    {
        s_Lua.reset();
    }

    // -------------------------------------------------------
    //  Bindings
    // -------------------------------------------------------

    void ScriptEngine::RegisterBindings()
    {
        sol::state& lua = *s_Lua;

        // ---- Vector3 ------------------------------------------------
        lua.new_usertype<glm::vec3>("Vector3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,
            sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
            sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
            sol::meta_function::multiplication, [](const glm::vec3& a, float s) { return a * s; },
            sol::meta_function::to_string, [](const glm::vec3& v) {
                return "(" + std::to_string(v.x) + ", " +
                    std::to_string(v.y) + ", " +
                    std::to_string(v.z) + ")"; }
        );

        // Vector3.new(x,y,z) convenience
        lua["Vector3"]["new"] = [](float x, float y, float z) { return glm::vec3(x, y, z); };
        lua["Vector3"]["zero"] = glm::vec3(0);
        lua["Vector3"]["one"] = glm::vec3(1);
        lua["Vector3"]["up"] = glm::vec3(0, 1, 0);
        lua["Vector3"]["forward"] = glm::vec3(0, 0, -1);
        lua["Vector3"]["right"] = glm::vec3(1, 0, 0);

        // ---- Entity -------------------------------------------------
        lua.new_usertype<Entity>("Entity",
            "GetTag", &Entity::GetTag,
            "SetTag", &Entity::SetTag,
            // Transform shortcuts
            "Position", sol::property(
                [](Entity& e) { return e.GetTransform().Position; },
                [](Entity& e, glm::vec3 v) { e.GetTransform().Position = v; }),
            "Rotation", sol::property(
                [](Entity& e) { return e.GetTransform().Rotation; },
                [](Entity& e, glm::vec3 v) { e.GetTransform().Rotation = v; }),
            "Scale", sol::property(
                [](Entity& e) { return e.GetTransform().Scale; },
                [](Entity& e, glm::vec3 v) { e.GetTransform().Scale = v; }),
            // Component helpers
            "HasMesh", &Entity::HasMesh,
            "HasLight", &Entity::HasLight,
            "HasMaterial", &Entity::HasMaterial
        );

        // ---- Input --------------------------------------------------
        auto inputTable = lua.create_table("Input");
        inputTable["IsKeyDown"] = [](int keycode) {
            return Input::IsKeyPressed(keycode);
            };
        inputTable["IsMouseButtonDown"] = [](int btn) {
            return Input::IsMouseButtonPressed(btn);
            };

        // Key constants (GLFW values, same as Roblox-style names)
        auto keyTable = lua.create_table("Key");
        keyTable["W"] = GLFW_KEY_W;     keyTable["A"] = GLFW_KEY_A;
        keyTable["S"] = GLFW_KEY_S;     keyTable["D"] = GLFW_KEY_D;
        keyTable["Q"] = GLFW_KEY_Q;     keyTable["E"] = GLFW_KEY_E;
        keyTable["Space"] = GLFW_KEY_SPACE; keyTable["LShift"] = GLFW_KEY_LEFT_SHIFT;
        keyTable["Up"] = GLFW_KEY_UP;    keyTable["Down"] = GLFW_KEY_DOWN;
        keyTable["Left"] = GLFW_KEY_LEFT;  keyTable["Right"] = GLFW_KEY_RIGHT;

        // ---- Log ----------------------------------------------------
        auto logTable = lua.create_table("Log");
        logTable["Info"] = [](const std::string& msg) { PD_CORE_INFO("[Lua] {0}", msg); };
        logTable["Warn"] = [](const std::string& msg) { PD_CORE_WARN("[Lua] {0}", msg); };
        logTable["Error"] = [](const std::string& msg) { PD_CORE_ERROR("[Lua] {0}", msg); };
    }

    // -------------------------------------------------------
    //  Play lifecycle
    // -------------------------------------------------------

    // Per-entity script environment stored alongside the component
    // We use a map keyed by entity ID since ScriptComponent can't hold sol objects
    // (sol types can't cross DLL boundaries safely as members)

    void ScriptEngine::OnPlayStart(Scene* scene)
    {
        s_ActiveScene = scene;
        s_Environments.clear();

        sol::state& lua = *s_Lua;

        // Re-register Scene table with live scene pointer
        auto sceneTable = lua.create_table("Scene");
        sceneTable["FindByName"] = [scene](const std::string& name) -> Entity* {
            return scene->FindEntityByName(name);
            };
        sceneTable["CreateEntity"] = [scene](const std::string& name) -> Entity* {
            return scene->CreateEntity(name);
            };
        lua["Scene"] = sceneTable;

        for (auto& ep : scene->GetEntities())
            RunEntityScript(ep.get(), scene);
    }

    void ScriptEngine::RunEntityScript(Entity* entity, Scene* scene)
    {
        if (!entity->HasScript()) return;
        auto* sc = entity->GetScript();
        if (sc->ScriptPath.empty()) return;

        // Read file
        std::ifstream f(sc->ScriptPath);
        if (!f.is_open())
        {
            sc->HasError = true;
            sc->ErrorMsg = "Cannot open: " + sc->ScriptPath;
            PD_CORE_ERROR("[Lua] {0}", sc->ErrorMsg);
            return;
        }
        std::string src((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());

        sol::state& lua = *s_Lua;

        // Create an isolated environment for this entity
        // so its OnCreate/OnUpdate don't collide with other entities' scripts
        sol::environment env(lua, sol::create, lua.globals());
        env["self"] = entity;

        // Load and run the script inside its environment
        auto result = lua.safe_script(src, env,
            [](lua_State*, sol::protected_function_result pfr) { return pfr; });

        if (!result.valid())
        {
            sol::error err = result;
            sc->HasError = true;
            sc->ErrorMsg = err.what();
            PD_CORE_ERROR("[Lua] Script error ({0}): {1}", entity->GetTag(), sc->ErrorMsg);
            return;
        }

        sc->HasError = false;
        sc->ErrorMsg = "";

        // Store environment so OnUpdate can call into it
        s_Environments[entity->GetID()] = env;

        // Call OnCreate if defined
        sol::protected_function onCreate = env["OnCreate"];
        if (onCreate.valid())
        {
            auto r = onCreate();
            if (!r.valid())
            {
                sol::error err = r;
                sc->HasError = true;
                sc->ErrorMsg = err.what();
                PD_CORE_ERROR("[Lua] OnCreate error ({0}): {1}", entity->GetTag(), sc->ErrorMsg);
            }
        }
    }

    void ScriptEngine::OnUpdate(Scene* scene, float dt)
    {
        if (!s_Lua || !scene) return;

        for (auto& ep : scene->GetEntities())
        {
            auto* e = ep.get();
            if (!e->HasScript()) continue;

            auto it = s_Environments.find(e->GetID());
            if (it == s_Environments.end()) continue;

            sol::environment& env = it->second;
            sol::protected_function onUpdate = env["OnUpdate"];
            if (!onUpdate.valid()) continue;

            auto r = onUpdate(dt);
            if (!r.valid())
            {
                sol::error err = r;
                auto* sc = e->GetScript();
                if (sc) { sc->HasError = true; sc->ErrorMsg = err.what(); }
                PD_CORE_ERROR("[Lua] OnUpdate error ({0}): {1}", e->GetTag(), err.what());
            }
        }
    }

    void ScriptEngine::OnPlayStop()
    {
        s_Environments.clear();
        s_ActiveScene = nullptr;
    }



} // namespace Pondo