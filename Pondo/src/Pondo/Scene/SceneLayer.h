#pragma once

#include <Pondo.h>
#include <glm/glm.hpp>
#include "EditorGeometry.h"
#include "Pondo/Renderer/RayUtils.h"
#include "Pondo/Renderer/Shaders.h"
#include "Pondo/Renderer/LightEnvironment.h"
#include "Pondo/Scene/Components.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>

#pragma warning(push)
#pragma warning(disable: 4251)
class PONDO_API SceneLayer : public Pondo::Layer
{
public:
    SceneLayer();
    std::unordered_map<uint32_t, Pondo::PointLightData> m_ObjectLights;
    struct AttachedLight
    {
        Pondo::Entity* Entity;
        Pondo::LightType Type;
    };
    std::vector<AttachedLight> m_AttachedLights;

    void NegateSelected();
    void SeparateSelected();

    // ---- Play state ------------------------------------------------
    enum class PlayState { Edit, Playing, Paused };

    void      Play();
    void      Pause();
    void      Stop();
    PlayState GetPlayState() const { return m_PlayState; }

    // ---- Framebuffer / viewport wiring -----------------------------
    void SetFramebuffer(std::shared_ptr<Pondo::Framebuffer> fb);
    void SetViewportFocused(bool v);
    void SetViewportRect(float x, float y, float w, float h);

    // ---- Snap settings ---------------------------------------------
    void SetSnapSettings(bool enabled, float move, float rotate, float scale);
    bool  m_EnableSnapping = false;
    float m_MoveIncrement = 0.5f;
    float m_RotateIncrement = 15.0f;
    float m_ScaleIncrement = 0.1f;

    // ---- Accessors -------------------------------------------------
    Pondo::Scene* GetScene() { return m_Scene.get(); }

    Pondo::Entity* GetSelectedEntity() const
    {
        return m_Selection.empty() ? nullptr : m_Selection[0];
    }
    const std::vector<Pondo::Entity*>& GetSelection() const { return m_Selection; }

    void SetSelectedEntity(Pondo::Entity* e)
    {
        m_Selection.clear();
        if (e) m_Selection.push_back(e);
    }
    void ToggleSelectedEntity(Pondo::Entity* e)
    {
        if (!e) return;
        auto it = std::find(m_Selection.begin(), m_Selection.end(), e);
        if (it != m_Selection.end()) m_Selection.erase(it);
        else                         m_Selection.push_back(e);
    }

    int   m_GizmoSpace = 0;
    float m_GizmoHintTimer = 0.0f;

    float GetGizmoHintTimer() const { return m_GizmoHintTimer; }
    void  TickGizmoHint(float dt)
    {
        m_GizmoHintTimer -= dt;
        if (m_GizmoHintTimer < 0.0f) m_GizmoHintTimer = 0.0f;
    }
    int GetGizmoSpace() const { return m_GizmoSpace; }

    Pondo::Camera* GetCamera() { return m_Camera.get(); }
    std::shared_ptr<Pondo::Shader> GetShader() { return m_Shader; }

    Pondo::LightEnvironment& GetLightEnvironment() { return m_Lights; }

    // ---- Scene operations ------------------------------------------
    void           LoadScene(const std::string& path);
    Pondo::Entity* SpawnEntity(const std::string& name,
        std::shared_ptr<Pondo::Mesh> mesh,
        glm::vec4 color,
        glm::vec3 pos = { 0, 0.5f, 0 });
    Pondo::Entity* SpawnLight(const std::string& name,
        Pondo::LightType type,
        glm::vec3 pos = { 0, 3.0f, 0 });
    Pondo::Entity* DuplicateEntity(Pondo::Entity* src);
    void           DeleteSelected();

    // ---- Picking ---------------------------------------------------
    void TryPickEntity(glm::vec2 px, bool addToSelection = false);

    // ---- Gizmo drag ------------------------------------------------
    int  GizmoAxisHit(glm::vec2 px);
    void BeginGizmoDrag(int axis, glm::vec2 px);
    void UpdateGizmoDrag(glm::vec2 px, bool enableSnap,
        float moveIncrement, float rotateIncrement,
        float scaleIncrement);
    bool EndGizmoDrag();
    bool IsDraggingGizmo() const { return m_DragAxis >= 0; }

    // ---- Layer overrides -------------------------------------------
    void OnAttach() override;
    void OnUpdate(Pondo::Timestep ts) override;
    void OnRender() override;
    void OnEvent(Pondo::Event& e) override;

    Pondo::Entity* CreateGroup(const std::string& name = "Group");
    void                        UngroupSelected();

    // Result codes for UnionGroup — lets EditorLayer show a user-facing error
    // instead of silently crashing or doing nothing.
    enum class UnionResult
    {
        OK,              // Union baked successfully
        PartialSuccess,  // Union complete but one or more negate ops failed
        NoSelection,     // Nothing selected
        NotAGroup,       // Selected entity is not a group root
        NoChildren,      // Group has no child entities
        NoPositiveParts, // All children are negated — nothing to union into
        MeshOperationFailed, // mcut returned an empty result
    };

    UnionResult                 UnionGroup();
    std::vector<Pondo::Entity*> GetGroupChildren(uint32_t groupRootID) const;

private:
    // ---- Play helpers ----------------------------------------------
    struct EntitySnapshot
    {
        std::string               Tag;
        Pondo::TransformComponent Transform;
        bool                      HasMesh = false;
        bool                      HasMaterial = false;
        bool                      HasLight = false;
        bool                      HasScript = false;
        glm::vec4                 MaterialColor = { 1,1,1,1 };
        Pondo::LightComponent     Light;
        Pondo::ScriptComponent    Script;
    };

    void TakeSnapshot();
    void RestoreSnapshot();

    PlayState                   m_PlayState = PlayState::Edit;
    std::vector<EntitySnapshot> m_SceneSnapshot;

    // ---- Rendering -------------------------------------------------
    void CollectLights();
    void DrawGizmo();
    void DrawArrow(const glm::vec3& origin, const glm::vec3& axis,
        float scale, const glm::vec4& color);
    void DrawRotationCircle(const glm::vec3& origin, const glm::vec3& axis,
        float scale, const glm::vec4& color);
    void DrawScaleDot(const glm::vec3& pos, float size, const glm::vec4& color);

    // ---- State members ---------------------------------------------
    std::shared_ptr<Pondo::Camera>      m_Camera;
    std::shared_ptr<Pondo::Shader>      m_Shader, m_FlatShader;
    std::shared_ptr<Pondo::Framebuffer> m_Framebuffer;
    std::unique_ptr<Pondo::Scene>       m_Scene;
    std::vector<Pondo::Entity*>         m_LightEntities;
    std::vector<Pondo::Entity*>         m_Selection;

    Pondo::LightEnvironment             m_Lights;

    GridRenderer m_Grid;
    ArrowGizmo   m_Arrow;

    bool      m_ViewportFocused = false;
    bool      m_FirstMouseLook = true;
    bool      m_Initialized = false;
    float     m_LastMouseX = 0;
    float     m_LastMouseY = 0;
    glm::vec2 m_VpPos = { 0, 0 };
    glm::vec2 m_VpSize = { 1280, 720 };

    int       m_DragAxis = -1;
    int       m_GizmoMode = 0;

    glm::vec2 m_DragStartPx = {};
    glm::vec3 m_DragStartPos = {};
    glm::vec3 m_DragStartScale = {};
    glm::vec3 m_DragStartRot = {};

    std::vector<glm::vec3> m_DragStartOffsets;
    std::vector<glm::vec3> m_DragStartScales;
    std::vector<glm::vec3> m_DragStartOffsetRots;
};
#pragma warning(pop)