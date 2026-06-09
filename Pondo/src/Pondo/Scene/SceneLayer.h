#pragma once

#include <Pondo.h>
#include <glm/glm.hpp>
#include "EditorGeometry.h"
#include "Pondo/Renderer/RayUtils.h"
#include "Pondo/Renderer/Shaders.h"
#include "Pondo/Renderer/LightEnvironment.h"
#include <unordered_map>
#include <vector>

// -------------------------------------------------------
//  SceneLayer
//  Owns the 3D scene, camera, gizmo interaction, and
//  offscreen framebuffer rendering.  The EditorLayer
//  drives it but never touches OpenGL directly.
// -------------------------------------------------------

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

    // --- Framebuffer / viewport wiring ---
    void SetFramebuffer(std::shared_ptr<Pondo::Framebuffer> fb);
    void SetViewportFocused(bool v);
    void SetViewportRect(float x, float y, float w, float h);

    // --- Snap settings ---
    void SetSnapSettings(bool enabled, float move, float rotate, float scale);
    bool  m_EnableSnapping  = false;
    float m_MoveIncrement   = 0.5f;
    float m_RotateIncrement = 15.0f;
    float m_ScaleIncrement  = 0.1f;

    // --- Accessors ---
    Pondo::Scene*                  GetScene()          { return m_Scene.get(); }
    Pondo::Entity*                 GetSelectedEntity() { return m_SelectedEntity; }
    void                           SetSelectedEntity(Pondo::Entity* e) { m_SelectedEntity = e; }
    Pondo::Camera*                 GetCamera()         { return m_Camera.get(); }
    std::shared_ptr<Pondo::Shader> GetShader()         { return m_Shader; }

    // --- Lighting environment (read/write by EditorLayer) ---
    Pondo::LightEnvironment&       GetLightEnvironment() { return m_Lights; }

    // --- Scene operations ---
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

    // --- Picking ---
    void TryPickEntity(glm::vec2 px);

    // --- Gizmo drag ---
    int  GizmoAxisHit(glm::vec2 px);
    void BeginGizmoDrag(int axis, glm::vec2 px);
    void UpdateGizmoDrag(glm::vec2 px, bool enableSnap,
                         float moveIncrement, float rotateIncrement, float scaleIncrement);
    bool EndGizmoDrag();
    bool IsDraggingGizmo() const { return m_DragAxis >= 0; }

    // --- Layer overrides ---
    void OnAttach() override;
    void OnUpdate(Pondo::Timestep ts) override;
    void OnRender() override;
    void OnEvent(Pondo::Event& e) override;

private:
    void CollectLights();   // builds m_Lights from entities each frame
    void DrawGizmo();
    void DrawArrow(const glm::vec3& origin, const glm::vec3& axis,
                   float scale, const glm::vec4& color);
    void DrawRotationCircle(const glm::vec3& origin, const glm::vec3& axis,
                            float scale, const glm::vec4& color);
    void DrawScaleDot(const glm::vec3& pos, float size, const glm::vec4& color);

    // --- State ---
    std::shared_ptr<Pondo::Camera>      m_Camera;
    std::shared_ptr<Pondo::Shader>      m_Shader, m_FlatShader;
    std::shared_ptr<Pondo::Framebuffer> m_Framebuffer;
    std::unique_ptr<Pondo::Scene>       m_Scene;
    std::vector<Pondo::Entity*> m_LightEntities;
    Pondo::Entity*                      m_SelectedEntity = nullptr;
    Pondo::LightEnvironment             m_Lights;   // rebuilt every frame

    GridRenderer m_Grid;
    ArrowGizmo   m_Arrow;

    bool      m_ViewportFocused = false;
    bool      m_FirstMouseLook  = true;
    bool      m_Initialized     = false;
    float     m_LastMouseX      = 0;
    float     m_LastMouseY      = 0;
    glm::vec2 m_VpPos  = { 0, 0 };
    glm::vec2 m_VpSize = { 1280, 720 };

    int m_DragAxis  = -1;
    int m_GizmoMode = 0;   // 0=Move  1=Rotate  2=Scale

    glm::vec2 m_DragStartPx    = {};
    glm::vec3 m_DragStartPos   = {};
    glm::vec3 m_DragStartScale = {};
    glm::vec3 m_DragStartRot   = {};
};
#pragma warning(pop)
