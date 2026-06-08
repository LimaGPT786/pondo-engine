#include <Pondo.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include "Pondo/Events/ApplicationEvents.h"
#include "Pondo/Events/KeyEvents.h"
#include "Pondo/Events/MouseEvents.h"
#include "Pondo/Scene/SceneSerializer.h"
#include <algorithm>
#include <type_traits>
#include <iostream>

#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

// No imgui_impl_glfw — we feed ImGuiIO ourselves so Pondo's GLFW
// callbacks are never touched.  Only the OpenGL *renderer* backend
// is used; everything else is manual.

// -------------------------------------------------------
//  Shaders
// -------------------------------------------------------

static const char* s_VertSrc = R"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec2 a_TexCoord;
    uniform mat4 u_ViewProjection;
    uniform mat4 u_Transform;
    out vec3 v_Normal;
    out vec2 v_TexCoord;
    void main() {
        v_Normal   = a_Normal;
        v_TexCoord = a_TexCoord;
        gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
    }
)";

static const char* s_FragSrc = R"(
    #version 450 core
    in  vec3 v_Normal;
    in  vec2 v_TexCoord;
    out vec4 FragColor;
    uniform vec4 u_Color;
    void main() {
        vec3  lightDir = normalize(vec3(0.8, 1.0, 0.6));
        float diff     = max(dot(normalize(v_Normal), lightDir), 0.0);
        float ambient  = 0.25;
        float lighting = ambient + diff * (1.0 - ambient);
        FragColor = vec4(u_Color.rgb * lighting, u_Color.a);
    }
)";

static const char* s_FlatVertSrc = R"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    uniform mat4 u_ViewProjection;
    uniform mat4 u_Transform;
    void main() {
        gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
    }
)";

static const char* s_FlatFragSrc = R"(
    #version 450 core
    out vec4 FragColor;
    uniform vec4 u_Color;
    void main() { FragColor = u_Color; }
)";

// -------------------------------------------------------
//  GLFW key code -> ImGuiKey
//  (ImGui 1.87+ requires AddKeyEvent; the old KeyMap is gone)
// -------------------------------------------------------

static ImGuiKey GlfwKeyToImGui(int key)
{
    switch (key)
    {
    case GLFW_KEY_TAB:          return ImGuiKey_Tab;
    case GLFW_KEY_LEFT:         return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT:        return ImGuiKey_RightArrow;
    case GLFW_KEY_UP:           return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN:         return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP:      return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN:    return ImGuiKey_PageDown;
    case GLFW_KEY_HOME:         return ImGuiKey_Home;
    case GLFW_KEY_END:          return ImGuiKey_End;
    case GLFW_KEY_INSERT:       return ImGuiKey_Insert;
    case GLFW_KEY_DELETE:       return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE:    return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE:        return ImGuiKey_Space;
    case GLFW_KEY_ENTER:        return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE:       return ImGuiKey_Escape;
    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_SHIFT:   return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_ALT:     return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER:   return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_CONTROL:return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_SHIFT:  return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_ALT:    return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER:  return ImGuiKey_RightSuper;
    case GLFW_KEY_A: return ImGuiKey_A; case GLFW_KEY_B: return ImGuiKey_B;
    case GLFW_KEY_C: return ImGuiKey_C; case GLFW_KEY_D: return ImGuiKey_D;
    case GLFW_KEY_E: return ImGuiKey_E; case GLFW_KEY_F: return ImGuiKey_F;
    case GLFW_KEY_G: return ImGuiKey_G; case GLFW_KEY_H: return ImGuiKey_H;
    case GLFW_KEY_I: return ImGuiKey_I; case GLFW_KEY_J: return ImGuiKey_J;
    case GLFW_KEY_K: return ImGuiKey_K; case GLFW_KEY_L: return ImGuiKey_L;
    case GLFW_KEY_M: return ImGuiKey_M; case GLFW_KEY_N: return ImGuiKey_N;
    case GLFW_KEY_O: return ImGuiKey_O; case GLFW_KEY_P: return ImGuiKey_P;
    case GLFW_KEY_Q: return ImGuiKey_Q; case GLFW_KEY_R: return ImGuiKey_R;
    case GLFW_KEY_S: return ImGuiKey_S; case GLFW_KEY_T: return ImGuiKey_T;
    case GLFW_KEY_U: return ImGuiKey_U; case GLFW_KEY_V: return ImGuiKey_V;
    case GLFW_KEY_W: return ImGuiKey_W; case GLFW_KEY_X: return ImGuiKey_X;
    case GLFW_KEY_Y: return ImGuiKey_Y; case GLFW_KEY_Z: return ImGuiKey_Z;
    case GLFW_KEY_0: return ImGuiKey_0; case GLFW_KEY_1: return ImGuiKey_1;
    case GLFW_KEY_2: return ImGuiKey_2; case GLFW_KEY_3: return ImGuiKey_3;
    case GLFW_KEY_4: return ImGuiKey_4; case GLFW_KEY_5: return ImGuiKey_5;
    case GLFW_KEY_6: return ImGuiKey_6; case GLFW_KEY_7: return ImGuiKey_7;
    case GLFW_KEY_8: return ImGuiKey_8; case GLFW_KEY_9: return ImGuiKey_9;
    case GLFW_KEY_F1:  return ImGuiKey_F1;  case GLFW_KEY_F2:  return ImGuiKey_F2;
    case GLFW_KEY_F3:  return ImGuiKey_F3;  case GLFW_KEY_F4:  return ImGuiKey_F4;
    case GLFW_KEY_F5:  return ImGuiKey_F5;  case GLFW_KEY_F6:  return ImGuiKey_F6;
    case GLFW_KEY_F7:  return ImGuiKey_F7;  case GLFW_KEY_F8:  return ImGuiKey_F8;
    case GLFW_KEY_F9:  return ImGuiKey_F9;  case GLFW_KEY_F10: return ImGuiKey_F10;
    case GLFW_KEY_F11: return ImGuiKey_F11; case GLFW_KEY_F12: return ImGuiKey_F12;
    default: return ImGuiKey_None;
    }
}

// -------------------------------------------------------
//  Ray helpers
// -------------------------------------------------------

struct Ray { glm::vec3 origin, dir; };

static float RayAABB(const Ray& ray, const glm::vec3& mn, const glm::vec3& mx)
{
    float tmin = 0.0f, tmax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        if (glm::abs(ray.dir[i]) < 1e-8f) {
            if (ray.origin[i] < mn[i] || ray.origin[i] > mx[i]) return -1.0f;
            continue;
        }
        float invD = 1.0f / ray.dir[i];
        float t0 = (mn[i] - ray.origin[i]) * invD;
        float t1 = (mx[i] - ray.origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax < tmin) return -1.0f;
    }
    return tmin;
}

static Ray ScreenToRay(const glm::vec2& px, const glm::vec2& vpPos,
    const glm::vec2& vpSize, const glm::mat4& invVP)
{
    float nx = ((px.x - vpPos.x) / vpSize.x) * 2.0f - 1.0f;
    float ny = 1.0f - ((px.y - vpPos.y) / vpSize.y) * 2.0f;
    glm::vec4 n = invVP * glm::vec4(nx, ny, -1.0f, 1.0f); n /= n.w;
    glm::vec4 f = invVP * glm::vec4(nx, ny, 1.0f, 1.0f); f /= f.w;
    return { glm::vec3(n), glm::normalize(glm::vec3(f) - glm::vec3(n)) };
}

static glm::vec2 WorldToScreen(const glm::vec3& world, const glm::mat4& vp,
    const glm::vec2& vpPos, const glm::vec2& vpSize)
{
    glm::vec4 clip = vp * glm::vec4(world, 1.0f);
    if (glm::abs(clip.w) < 1e-6f) return { -9999, -9999 };
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    return {
        vpPos.x + (ndc.x * 0.5f + 0.5f) * vpSize.x,
        vpPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y
    };
}

// -------------------------------------------------------
//  Grid geometry
// -------------------------------------------------------

struct GridRenderer {
    std::shared_ptr<Pondo::VertexArray> VA;
    int lineCount = 0;

    void Build(int halfSize, int step) {
        std::vector<float> v;
        for (int i = -halfSize; i <= halfSize; i += step) {
            float f = (float)i;
            v.insert(v.end(), { f,0,-(float)halfSize, f,0,(float)halfSize });
            v.insert(v.end(), { -(float)halfSize,0,f, (float)halfSize,0,f });
        }
        lineCount = (int)v.size() / 3;
        auto vb = std::make_shared<Pondo::VertexBuffer>(v.data(), (unsigned int)(v.size() * sizeof(float)));
        VA = std::make_shared<Pondo::VertexArray>();
        VA->AddVertexBuffer(vb);
    }
    void Draw() const {
        VA->Bind();
        glDrawArrays(GL_LINES, 0, lineCount);
        VA->Unbind();
    }
};

// -------------------------------------------------------
//  Arrow gizmo geometry
// -------------------------------------------------------

struct ArrowGizmo {
    std::shared_ptr<Pondo::VertexArray> VA;
    int vertCount = 0;

    void Build() {
        float s = 0.07f, tip = 1.0f, base = 0.78f;
        std::vector<float> v = {
            0,0,0,  0,tip,0,
            0,tip,0,  s,base,0,    0,tip,0, -s,base,0,
            0,tip,0,  0,base,s,    0,tip,0,  0,base,-s,
        };
        vertCount = (int)v.size() / 3;
        auto vb = std::make_shared<Pondo::VertexBuffer>(v.data(), (unsigned int)(v.size() * sizeof(float)));
        VA = std::make_shared<Pondo::VertexArray>();
        VA->AddVertexBuffer(vb);
    }
    void Draw() const {
        VA->Bind();
        glDrawArrays(GL_LINES, 0, vertCount);
        VA->Unbind();
    }
};

class ICommand
{
public:
    virtual ~ICommand() = default;

    virtual void Undo() = 0;
    virtual void Redo() = 0;
};

class TransformCommand
    : public ICommand
{
public:

    using TransformType =
        decltype(
            std::declval<Pondo::Entity>()
            .GetTransform()
            );

    TransformCommand(
        Pondo::Entity* entity,
        const TransformType& before,
        const TransformType& after
    )
        :
        m_Entity(entity),
        m_Before(before),
        m_After(after)
    {
    }

    void Undo() override
    {
        if (m_Entity)
        {
            m_Entity
                ->GetTransform()
                = m_Before;
        }
    }

    void Redo() override
    {
        if (m_Entity)
        {
            m_Entity
                ->GetTransform()
                = m_After;
        }
    }

private:

    Pondo::Entity* m_Entity;

    TransformType m_Before;
    TransformType m_After;
};

// -------------------------------------------------------
//  SceneLayer
// -------------------------------------------------------

class SceneLayer : public Pondo::Layer {
public:
    SceneLayer()
        : Layer("SceneLayer")
    {
    }

    void SetFramebuffer(
        std::shared_ptr<Pondo::Framebuffer> fb
    )
    {
        m_Framebuffer = fb;
    }

    void SetViewportFocused(bool v)
    {
        m_ViewportFocused = v;
    }

    void SetViewportRect(
        float x,
        float y,
        float w,
        float h
    )
    {
        m_VpPos = { x, y };
        m_VpSize = { w, h };
    }

    void SetSnapSettings(
        bool enabled,
        float move,
        float rotate,
        float scale
    )
    {
        m_EnableSnapping = enabled;
        m_MoveIncrement = move;
        m_RotateIncrement = rotate;
        m_ScaleIncrement = scale;
    }

    void LoadScene(const std::string& path)
    {
        m_SelectedEntity = nullptr;
        m_Scene->Clear();
        Pondo::SceneSerializer::Load(m_Scene.get(), path, m_Shader);

        auto& entities = m_Scene->GetEntities();
        if (!entities.empty())
            m_SelectedEntity = entities[0].get();
    }

    bool m_EnableSnapping = false;

    float m_MoveIncrement = 0.5f;
    float m_RotateIncrement = 15.0f;
    float m_ScaleIncrement = 0.1f;

    Pondo::Scene* GetScene() { return m_Scene.get(); }
    Pondo::Entity* GetSelectedEntity() { return m_SelectedEntity; }
    void           SetSelectedEntity(Pondo::Entity* e) { m_SelectedEntity = e; }
    Pondo::Camera* GetCamera() { return m_Camera.get(); }
    std::shared_ptr<Pondo::Shader> GetShader() { return m_Shader; }

    void TryPickEntity(glm::vec2 px) {
        if (m_VpSize.x <= 0 || m_VpSize.y <= 0) return;
        Ray ray = ScreenToRay(px, m_VpPos, m_VpSize,
            glm::inverse(m_Camera->GetViewProjection()));
        Pondo::Entity* best = nullptr; float bestT = 1e30f;
        for (auto& ep : m_Scene->GetEntities()) {
            if (!ep->GetMesh()) continue;
            glm::vec3 c = ep->GetTransform().Position;
            glm::vec3 h = glm::abs(ep->GetTransform().Scale) * 0.5f;
            float t = RayAABB(ray, c - h, c + h);
            if (t > 0.0f && t < bestT) { bestT = t; best = ep.get(); }
        }
        m_SelectedEntity = best;
    }

    // Hit-test along the whole shaft (not just the tip) with generous radius
    int GizmoAxisHit(glm::vec2 px)
    {
        if (!m_SelectedEntity)
            return -1;

        glm::vec3 origin =
            m_SelectedEntity->GetTransform().Position;

        float gizmoScale =
            glm::length(
                m_Camera->GetPosition() -
                origin
            ) * 0.15f;

        // =========================
        // ROTATION MODE
        // =========================
        if (m_GizmoMode == 1)
        {
            int bestAxis = -1;
            float bestDist = 999999.0f;

            for (int axis = 0; axis < 3; axis++)
            {
                const int segments = 64;

                for (int i = 0; i < segments; i++)
                {
                    float a0 =
                        glm::two_pi<float>() *
                        ((float)i / segments);

                    float a1 =
                        glm::two_pi<float>() *
                        ((float)(i + 1) / segments);

                    glm::vec3 p0;
                    glm::vec3 p1;

                    if (axis == 0)
                    {
                        p0 = origin + glm::vec3(
                            0,
                            cos(a0),
                            sin(a0)
                        ) * gizmoScale;

                        p1 = origin + glm::vec3(
                            0,
                            cos(a1),
                            sin(a1)
                        ) * gizmoScale;
                    }
                    else if (axis == 1)
                    {
                        p0 = origin + glm::vec3(
                            cos(a0),
                            0,
                            sin(a0)
                        ) * gizmoScale;

                        p1 = origin + glm::vec3(
                            cos(a1),
                            0,
                            sin(a1)
                        ) * gizmoScale;
                    }
                    else
                    {
                        p0 = origin + glm::vec3(
                            cos(a0),
                            sin(a0),
                            0
                        ) * gizmoScale;

                        p1 = origin + glm::vec3(
                            cos(a1),
                            sin(a1),
                            0
                        ) * gizmoScale;
                    }

                    glm::vec2 s0 =
                        WorldToScreen(
                            p0,
                            m_Camera->GetViewProjection(),
                            m_VpPos,
                            m_VpSize
                        );

                    glm::vec2 s1 =
                        WorldToScreen(
                            p1,
                            m_Camera->GetViewProjection(),
                            m_VpPos,
                            m_VpSize
                        );

                    glm::vec2 seg =
                        s1 - s0;

                    float len =
                        glm::dot(seg, seg);

                    if (len < 1.0f)
                        continue;

                    float t =
                        glm::clamp(
                            glm::dot(px - s0, seg) / len,
                            0.0f,
                            1.0f
                        );

                    glm::vec2 hit =
                        s0 + seg * t;

                    float dist =
                        glm::length(
                            px - hit
                        );

                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestAxis = axis;
                    }
                }
            }

            return bestDist < 40.0f
                ? bestAxis
                : -1;
        }

        // =========================
        // MOVE + SCALE MODE
        // =========================

        glm::vec3 tips[3] =
        {
            origin + glm::vec3(gizmoScale,0,0),
            origin + glm::vec3(0,gizmoScale,0),
            origin + glm::vec3(0,0,gizmoScale)
        };

        int bestAxis = -1;
        float bestDist = 999999.0f;

        for (int i = 0; i < 3; i++)
        {
            glm::vec2 a =
                WorldToScreen(
                    origin,
                    m_Camera->GetViewProjection(),
                    m_VpPos,
                    m_VpSize
                );

            glm::vec2 b =
                WorldToScreen(
                    tips[i],
                    m_Camera->GetViewProjection(),
                    m_VpPos,
                    m_VpSize
                );

            glm::vec2 ab =
                b - a;

            float len =
                glm::dot(ab, ab);

            if (len < 1.0f)
                continue;

            float t =
                glm::clamp(
                    glm::dot(px - a, ab) / len,
                    0.0f,
                    1.0f
                );

            glm::vec2 hit =
                a + ab * t;

            float dist =
                glm::length(
                    px - hit
                );

            if (dist < bestDist)
            {
                bestDist = dist;
                bestAxis = i;
            }
        }

        return bestDist < 35.0f
            ? bestAxis
            : -1;
    }

    void BeginGizmoDrag(int axis, glm::vec2 px)
    {
        if (!m_SelectedEntity)
            return;

        m_DragAxis =
            axis;

        m_DragStartPx =
            px;

        auto& tf =
            m_SelectedEntity
            ->GetTransform();

        m_DragStartPos =
            tf.Position;

        m_DragStartScale =
            tf.Scale;

        m_DragStartRot =
            tf.Rotation;
    }

    void UpdateGizmoDrag(
        glm::vec2 px,
        bool enableSnap,
        float moveIncrement,
        float rotateIncrement,
        float scaleIncrement
    )

    {
        if (
            m_DragAxis < 0 ||
            !m_SelectedEntity
            )
            return;

        auto& transform =
            m_SelectedEntity
            ->GetTransform();

        static const glm::vec3 axes[3] =
        {
            {1,0,0},
            {0,1,0},
            {0,0,1}
        };

        glm::vec3 axis =
            axes[m_DragAxis];

        glm::vec2 p0 =
            WorldToScreen(
                m_DragStartPos,
                m_Camera->GetViewProjection(),
                m_VpPos,
                m_VpSize
            );

        glm::vec2 p1 =
            WorldToScreen(
                m_DragStartPos + axis,
                m_Camera->GetViewProjection(),
                m_VpPos,
                m_VpSize
            );

        glm::vec2 dir =
            p1 - p0;

        float len =
            glm::length(dir);

        if (len < 1.0f)
            return;

        dir /= len;

        float dragAmount =
            glm::dot(
                px - m_DragStartPx,
                dir
            )
            * 0.01f;

        switch (m_GizmoMode)
        {
        case 0:
        {
            if (
                enableSnap &&
                moveIncrement > 0.0f
                )
            {
                dragAmount =
                    round(
                        dragAmount
                        /
                        moveIncrement
                    )
                    *
                    moveIncrement;
            }

            transform.Position =
                m_DragStartPos
                +
                axis
                *
                dragAmount;

            break;
        }

        case 1:
        {
            float rotation =
                dragAmount
                *
                100.0f;

            if (
                enableSnap &&
                rotateIncrement > 0.0f
                )
            {
                rotation =
                    round(
                        rotation
                        /
                        rotateIncrement
                    )
                    *
                    rotateIncrement;
            }

            transform.Rotation[m_DragAxis] =
                m_DragStartRot[m_DragAxis]
                +
                rotation;

            break;
        }

        case 2:
        {
            if (
                enableSnap &&
                scaleIncrement > 0.0f
                )
            {
                dragAmount =
                    round(
                        dragAmount
                        /
                        scaleIncrement
                    )
                    *
                    scaleIncrement;
            }

            transform.Scale[m_DragAxis] =
                std::max(
                    0.001f,
                    m_DragStartScale[m_DragAxis]
                    +
                    dragAmount
                );

            break;
        }
        }
    }

    bool EndGizmoDrag()
    {
        bool dragged =
            m_DragAxis >= 0;

        m_DragAxis = -1;

        return dragged;
    }
    bool IsDraggingGizmo() const { return m_DragAxis >= 0; }

    Pondo::Entity* DuplicateEntity(
        Pondo::Entity* src
    )
    {
        if (!src)
            return nullptr;

        glm::vec4 color =
        {
            1,1,1,1
        };

        if (
            src->GetMaterial() &&
            src->GetMaterial()->Mat
            )
        {
            color =
                src
                ->GetMaterial()
                ->Mat
                ->GetColor();
        }

        auto* copy =
            SpawnEntity(
                src->GetTag() + "_Copy",
                src->GetMesh()->MeshData,
                color,
                src->GetTransform().Position
            );

        copy->GetTransform() =
            src->GetTransform();

        return copy;
    }

    Pondo::Entity* SpawnEntity(
        const std::string& name,
        std::shared_ptr<Pondo::Mesh> mesh,
        glm::vec4 color,
        glm::vec3 pos = { 0,0.5f,0 }
    )
    {
        auto* e = m_Scene->CreateEntity(name);

        e->GetTransform().Position = pos;

        e->SetMesh(mesh);

        e->SetMaterial(
            std::make_shared<Pondo::Material>(
                m_Shader
            )
        );

        e->GetMaterial()
            ->Mat
            ->SetColor(color);

        return e;
    }

    void DeleteSelected() {
        if (!m_SelectedEntity) return;
        uint32_t id = m_SelectedEntity->GetID();
        m_SelectedEntity = nullptr;
        m_Scene->DestroyEntity(id);
    }

    void OnAttach() override {
        m_Camera = std::make_shared<Pondo::Camera>(60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        m_Shader = std::make_shared<Pondo::Shader>(s_VertSrc, s_FragSrc);
        m_FlatShader = std::make_shared<Pondo::Shader>(s_FlatVertSrc, s_FlatFragSrc);
        m_Scene = std::make_unique<Pondo::Scene>("Main Scene");

        SpawnEntity("Cube", Pondo::Mesh::CreateCube(), { 0.8f,0.35f,0.2f,1 }, { 0,0.5f,0 });
        SpawnEntity("Sphere", Pondo::Mesh::CreateSphere(), { 0.2f,0.55f,0.85f,1 }, { 2.5f,0.5f,0 });

        auto* ground = m_Scene->CreateEntity("Ground");
        ground->GetTransform().Scale = { 20,1,20 };
        ground->GetTransform().Position = { 0,-0.5f,0 };
        ground->SetMesh(Pondo::Mesh::CreatePlane(1.0f));
        ground->SetMaterial(std::make_shared<Pondo::Material>(m_Shader));
        ground->GetMaterial()->Mat->SetColor({ 0.3f,0.5f,0.3f,1 });
    }

    void OnUpdate(Pondo::Timestep ts) override {
        // Block camera movement while ImGui owns the keyboard or mouse
        if (!m_ViewportFocused)
        {
            m_FirstMouseLook = true;
            return;
        }

        float spd = 5.0f * ts.GetSeconds();
        float sens = 50.0f * ts.GetSeconds();
        glm::vec3 pos = m_Camera->GetPosition();
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_W)) pos += m_Camera->GetForward() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_S)) pos -= m_Camera->GetForward() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_A)) pos -= m_Camera->GetRight() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_D)) pos += m_Camera->GetRight() * spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_E)) pos.y += spd;
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_Q)) pos.y -= spd;
        m_Camera->SetPosition(pos);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_LEFT))  m_Camera->SetYaw(m_Camera->GetYaw() - sens);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_RIGHT)) m_Camera->SetYaw(m_Camera->GetYaw() + sens);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_UP))    m_Camera->SetPitch(m_Camera->GetPitch() + sens);
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_DOWN))  m_Camera->SetPitch(m_Camera->GetPitch() - sens);

        if (Pondo::Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT) && !IsDraggingGizmo()) {
            auto [mx, my] = Pondo::Input::GetMousePosition();
            if (m_FirstMouseLook) { m_LastMouseX = mx; m_LastMouseY = my; m_FirstMouseLook = false; }
            m_Camera->ProcessMouseLook(mx - m_LastMouseX, my - m_LastMouseY);
            m_LastMouseX = mx; m_LastMouseY = my;
        }
        else { m_FirstMouseLook = true; }
    }

    void OnRender() override {
        if (!m_Framebuffer) return;

        if (!m_Initialized) {
            m_Grid.Build(50, 1);
            m_Arrow.Build();
            m_Initialized = true;
        }

        m_Framebuffer->Bind();
        Pondo::Renderer::SetClearColor(0.12f, 0.12f, 0.13f, 1.0f);
        Pondo::Renderer::Clear();
        Pondo::Renderer::BeginScene(*m_Camera);

        glEnable(GL_DEPTH_TEST);
        for (auto& ep : m_Scene->GetEntities()) {
            auto* mc = ep->GetMesh(); auto* mat = ep->GetMaterial();
            if (!mc || !mat || !mc->MeshData || !mat->Mat) continue;
            Pondo::Renderer::SubmitMesh(mc->MeshData, mat->Mat, ep->GetTransform().GetTransform());
        }

        m_FlatShader->Bind();
        m_FlatShader->SetMat4("u_ViewProjection", m_Camera->GetViewProjection());
        m_FlatShader->SetMat4("u_Transform", glm::mat4(1.0f));
        m_FlatShader->SetFloat4("u_Color", { 0.35f,0.35f,0.38f,1 });
        glLineWidth(1.0f); m_Grid.Draw();

        if (m_SelectedEntity) {
            auto& tf = m_SelectedEntity->GetTransform();
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            m_FlatShader->Bind();
            m_FlatShader->SetMat4("u_ViewProjection", m_Camera->GetViewProjection());
            glm::mat4 transform = tf.GetTransform();
            transform = transform * glm::scale(glm::mat4(1), glm::vec3(1.05f));
            m_FlatShader->SetMat4("u_Transform", transform);
            m_FlatShader->SetFloat4("u_Color", { 1,0.85f,0.1f,1 });
            auto* mc = m_SelectedEntity->GetMesh();
            if (mc && mc->MeshData) {
                mc->MeshData->Bind();
                glDrawElements(GL_TRIANGLES, mc->MeshData->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
                mc->MeshData->Unbind();
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            DrawGizmo();
            glEnable(GL_DEPTH_TEST);
        }

        Pondo::Renderer::EndScene();
        m_Framebuffer->Unbind();
    }

    void OnEvent(Pondo::Event& e) override {
        Pondo::EventDispatcher d(e);
        d.Dispatch<Pondo::WindowResizeEvent>([this](Pondo::WindowResizeEvent& ev) {
            if (ev.GetWidth() > 0 && ev.GetHeight() > 0)
                m_Camera->SetAspectRatio((float)ev.GetWidth() / ev.GetHeight());
            return false;
            });
    }

private:
    void DrawGizmo()
    {
        if (!m_SelectedEntity)
            return;

        // Move / Rotate / Scale
        if (Pondo::Input::IsKeyPressed(GLFW_KEY_1))
            m_GizmoMode = 0;

        if (Pondo::Input::IsKeyPressed(GLFW_KEY_2))
            m_GizmoMode = 1;

        if (Pondo::Input::IsKeyPressed(GLFW_KEY_3))
            m_GizmoMode = 2;

        glm::vec3 origin =
            m_SelectedEntity
            ->GetTransform()
            .Position;

        float scale =
            glm::length(
                m_Camera->GetPosition()
                - origin
            ) * 0.15f;

        glDisable(GL_DEPTH_TEST);

        struct Axis
        {
            glm::vec3 dir;
            glm::vec4 color;
            glm::vec4 active;
        };

        Axis axes[3] =
        {
            {{1,0,0},{1,0.2f,0.2f,1},{1,0.6f,0.6f,1}},
            {{0,1,0},{0.2f,1,0.2f,1},{0.6f,1,0.6f,1}},
            {{0,0,1},{0.2f,0.4f,1,1},{0.6f,0.8f,1,1}}
        };

        for (int i = 0; i < 3; i++)
        {
            glm::vec4 col =
                (m_DragAxis == i)
                ? axes[i].active
                : axes[i].color;

            glm::vec3 tip =
                origin +
                axes[i].dir *
                scale;

            //--------------------------------------------------
            // MOVE → arrows
            //--------------------------------------------------

            if (m_GizmoMode == 0)
            {
                DrawArrow(
                    origin,
                    axes[i].dir,
                    scale,
                    col
                );
            }

            //--------------------------------------------------
            // ROTATE → axis lines
            //--------------------------------------------------

            else if (m_GizmoMode == 1)
            {
                DrawRotationCircle(
                    origin,
                    axes[i].dir,
                    scale,
                    col
                );
            }

            //--------------------------------------------------
            // SCALE → dots
            //--------------------------------------------------

            else
            {
                DrawScaleDot(
                    tip,
                    scale,
                    col
                );
            }
        }

        glEnable(GL_DEPTH_TEST);
    }

    void DrawArrow(const glm::vec3& origin, const glm::vec3& axis,
        float scale, const glm::vec4& color)
    {
        glm::vec3 up = { 0,1,0 };
        glm::mat4 rot = glm::mat4(1);
        float d = glm::dot(up, axis);
        if (d < -0.9999f)     rot = glm::rotate(glm::mat4(1), glm::pi<float>(), glm::vec3(1, 0, 0));
        else if (d < 0.9999f) rot = glm::rotate(glm::mat4(1), glm::acos(glm::clamp(d, -1.f, 1.f)),
            glm::normalize(glm::cross(up, axis)));
        glm::mat4 t = glm::translate(glm::mat4(1), origin) * rot * glm::scale(glm::mat4(1), glm::vec3(scale));
        m_FlatShader->SetMat4("u_Transform", t);
        m_FlatShader->SetFloat4("u_Color", color);
        m_Arrow.Draw();
    }

    void DrawRotationCircle(
        const glm::vec3& origin,
        const glm::vec3& axis,
        float scale,
        const glm::vec4& color)
    {
        constexpr int SEGMENTS = 48;

        std::vector<float> verts;

        float radius =
            scale * 0.9f;

        glm::vec3 right;
        glm::vec3 up;

        // choose plane perpendicular to axis
        if (axis.x != 0)
        {
            right = { 0,1,0 };
            up = { 0,0,1 };
        }
        else if (axis.y != 0)
        {
            right = { 1,0,0 };
            up = { 0,0,1 };
        }
        else
        {
            right = { 1,0,0 };
            up = { 0,1,0 };
        }

        for (int i = 0; i <= SEGMENTS; i++)
        {
            float a =
                glm::two_pi<float>() *
                ((float)i / SEGMENTS);

            glm::vec3 p =
                origin +
                right * cos(a) * radius +
                up * sin(a) * radius;

            verts.push_back(p.x);
            verts.push_back(p.y);
            verts.push_back(p.z);
        }

        auto vb =
            std::make_shared<Pondo::VertexBuffer>(
                verts.data(),
                (uint32_t)(
                    verts.size()
                    * sizeof(float)
                    )
            );

        auto va =
            std::make_shared<Pondo::VertexArray>();

        va->AddVertexBuffer(vb);

        m_FlatShader->Bind();

        m_FlatShader->SetMat4(
            "u_Transform",
            glm::mat4(1)
        );

        m_FlatShader->SetFloat4(
            "u_Color",
            color
        );

        va->Bind();

        glLineWidth(4);

        glDrawArrays(
            GL_LINE_STRIP,
            0,
            SEGMENTS + 1
        );

        va->Unbind();

        glLineWidth(1);
    }

    void DrawScaleDot(
        const glm::vec3& pos,
        float size,
        const glm::vec4& color)
    {
        m_FlatShader->Bind();

        glm::mat4 transform =
            glm::translate(
                glm::mat4(1.0f),
                pos
            ) *
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(size * 0.08f)
            );

        m_FlatShader->SetMat4(
            "u_Transform",
            transform
        );

        m_FlatShader->SetFloat4(
            "u_Color",
            color
        );

        glPointSize(18.0f);

        m_Arrow.VA->Bind();

        glDrawArrays(
            GL_POINTS,
            0,
            1
        );

        m_Arrow.VA->Unbind();

        glPointSize(1.0f);
    }

    std::shared_ptr<Pondo::Camera>      m_Camera;
    std::shared_ptr<Pondo::Shader>      m_Shader, m_FlatShader;
    std::shared_ptr<Pondo::Framebuffer> m_Framebuffer;
    std::unique_ptr<Pondo::Scene>       m_Scene;
    Pondo::Entity* m_SelectedEntity = nullptr;
    GridRenderer m_Grid; ArrowGizmo m_Arrow;
    bool m_ViewportFocused = false, m_FirstMouseLook = true, m_Initialized = false;
    float m_LastMouseX = 0, m_LastMouseY = 0;
    glm::vec2 m_VpPos = { 0,0 }, m_VpSize = { 1280,720 };
    int m_DragAxis = -1;

    // 0 Move
    // 1 Rotate
    // 2 Scale
    int m_GizmoMode = 0;

    glm::vec2 m_DragStartPx = {};

    glm::vec3 m_DragStartPos = {};
    glm::vec3 m_DragStartScale = {};
    glm::vec3 m_DragStartRot = {};
};

static std::string OpenFileDialog()
{
    char path[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "Pondo Scene\0*.pondo\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "pondo";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
        return std::string(path);

    return "";
}

static std::string SaveFileDialog()
{
    char path[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "Pondo Scene\0*.pondo\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "pondo";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn))
        return std::string(path);

    return "";
}

// -------------------------------------------------------
//  EditorLayer
//
//  Pure manual ImGuiIO — zero GLFW backend.
//  Mouse polling stays identical to your original (which
//  already worked for picking/dragging).
//  The only additions are AddKeyEvent() and AddInputCharacter()
//  forwarded through Pondo's own event system — the two pieces
//  that were missing and caused buttons/inputs to be dead.
// -------------------------------------------------------

class EditorLayer : public Pondo::Layer {
public:
    EditorLayer(SceneLayer* sl) : Layer("EditorLayer"), m_SceneLayer(sl) {}

    void OnAttach() override {
        m_Window = static_cast<GLFWwindow*>(
            Pondo::Application::Get().GetWindow().GetNativeWindow());

        int w = Pondo::Application::Get().GetWindow().GetWidth();
        int h = Pondo::Application::Get().GetWindow().GetHeight();

        Pondo::FramebufferSpec spec;
        spec.Width = (unsigned)w; spec.Height = (unsigned)h;
        m_Framebuffer = std::make_shared<Pondo::Framebuffer>(spec);
        m_SceneLayer->SetFramebuffer(m_Framebuffer);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetMainViewport()->PlatformHandleRaw = glfwGetWin32Window(m_Window);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.DisplaySize = { (float)w, (float)h };
        io.DisplayFramebufferScale = { 1, 1 };
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendPlatformName = "pondo_manual";

        ImGui::StyleColorsDark();
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 4.0f; s.FrameRounding = 3.0f;
        s.Colors[ImGuiCol_WindowBg] = { 0.13f,0.13f,0.14f,1 };
        s.Colors[ImGuiCol_TitleBg] = { 0.10f,0.10f,0.11f,1 };
        s.Colors[ImGuiCol_TitleBgActive] = { 0.16f,0.29f,0.48f,1 };
        s.Colors[ImGuiCol_Header] = { 0.20f,0.35f,0.55f,0.6f };
        s.Colors[ImGuiCol_HeaderHovered] = { 0.26f,0.47f,0.70f,0.8f };
        s.Colors[ImGuiCol_FrameBg] = { 0.18f,0.18f,0.20f,1 };
        s.Colors[ImGuiCol_Button] = { 0.20f,0.35f,0.55f,1 };
        s.Colors[ImGuiCol_ButtonHovered] = { 0.26f,0.47f,0.70f,1 };

        // Only the OpenGL renderer backend — no GLFW backend at all
        ImGui_ImplOpenGL3_Init("#version 450");
    }

    void OnDetach() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }

    // Forward Pondo events into ImGui.
    // Mouse buttons, scroll, keyboard, and typed characters all come
    // through here — Pondo's own callbacks are untouched.
    void OnEvent(Pondo::Event& e) override {
        ImGuiIO& io = ImGui::GetIO();
        Pondo::EventDispatcher d(e);

        d.Dispatch<Pondo::WindowResizeEvent>([&io](Pondo::WindowResizeEvent& ev) {
            io.DisplaySize = { (float)ev.GetWidth(), (float)ev.GetHeight() };
            return false;
            });

        d.Dispatch<Pondo::MouseButtonPressedEvent>([&io](Pondo::MouseButtonPressedEvent& ev) {
            int btn = ev.GetMouseButton();
            if (btn >= 0 && btn < 5) io.AddMouseButtonEvent(btn, true);
            return false;
            });

        d.Dispatch<Pondo::MouseButtonReleasedEvent>([&io](Pondo::MouseButtonReleasedEvent& ev) {
            int btn = ev.GetMouseButton();
            if (btn >= 0 && btn < 5) io.AddMouseButtonEvent(btn, false);
            return false;
            });

        d.Dispatch<Pondo::MouseScrolledEvent>([&io](Pondo::MouseScrolledEvent& ev) {
            io.AddMouseWheelEvent(ev.GetXOffset(), ev.GetYOffset());
            return false;
            });

        d.Dispatch<Pondo::KeyPressedEvent>([this, &io](Pondo::KeyPressedEvent& ev)
            {
                ImGuiKey key = GlfwKeyToImGui(ev.GetKeyCode());

                if (key != ImGuiKey_None)
                    io.AddKeyEvent(key, true);

                io.AddKeyEvent(ImGuiMod_Ctrl,
                    Pondo::Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
                    Pondo::Input::IsKeyPressed(GLFW_KEY_RIGHT_CONTROL));

                io.AddKeyEvent(ImGuiMod_Shift,
                    Pondo::Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                    Pondo::Input::IsKeyPressed(GLFW_KEY_RIGHT_SHIFT));

                io.AddKeyEvent(ImGuiMod_Alt,
                    Pondo::Input::IsKeyPressed(GLFW_KEY_LEFT_ALT) ||
                    Pondo::Input::IsKeyPressed(GLFW_KEY_RIGHT_ALT));

                return false;
            });

        d.Dispatch<Pondo::KeyReleasedEvent>([&io](Pondo::KeyReleasedEvent& ev) {
            ImGuiKey imkey = GlfwKeyToImGui(ev.GetKeyCode());
            if (imkey != ImGuiKey_None) io.AddKeyEvent(imkey, false);
            return false;
            });

        // KeyTypedEvent carries the Unicode codepoint — feeds InputText
        d.Dispatch<Pondo::KeyTypedEvent>([&io](Pondo::KeyTypedEvent& ev) {
            io.AddInputCharacter((unsigned int)ev.GetKeyCode());
            return false;
            });
    }


    void OnRender() override {
        int w = Pondo::Application::Get().GetWindow().GetWidth();
        int h = Pondo::Application::Get().GetWindow().GetHeight();

        ImGuiIO& io = ImGui::GetIO();

        // ── Feed ImGuiIO for this frame ───────────────────────────────
        io.DisplaySize = { (float)w, (float)h };
        io.DisplayFramebufferScale = { 1, 1 };

        float now = (float)glfwGetTime();
        io.DeltaTime = (m_LastTime > 0.0f) ? (now - m_LastTime) : (1.0f / 60.0f);
        if (io.DeltaTime <= 0.0f) io.DeltaTime = 1.0f / 60.0f;
        m_LastTime = now;

        // Mouse position — poll directly, same as your original
        {
            auto [mx, my] = Pondo::Input::GetMousePosition();
            io.AddMousePosEvent(mx, my);
        }

        // ── Begin frame ───────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

		// Ctrl+Z / Ctrl+Y for undo/redo — only if ImGui doesn't already want the keyboard
        if (
            io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Z, false)
            )
        {
            Undo();
        }

        if (
            io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Y, false)
            )
        {
            Redo();
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
        {
            if (m_CurrentScenePath.empty())
                m_CurrentScenePath = SaveFileDialog();

            if (!m_CurrentScenePath.empty())
                Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), m_CurrentScenePath);
        }

        // ── Menu bar ─────────────────────────────────────────────────
        ImGui::SetNextWindowPos({ 0,0 });
        ImGui::SetNextWindowSize({ (float)w, 0 });
        ImGui::SetNextWindowBgAlpha(0.95f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("##Menu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar(2);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save Scene"))
                {
                    std::string path = SaveFileDialog();
                    if (!path.empty())
                        Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), path);
                }

                if (ImGui::MenuItem("Load Scene"))
                {
                    std::string path = OpenFileDialog();
                    if (!path.empty())
                        m_SceneLayer->LoadScene(path);
                }

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    if (m_CurrentScenePath.empty())
                        m_CurrentScenePath = SaveFileDialog();

                    if (!m_CurrentScenePath.empty())
                        Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), m_CurrentScenePath);
                }

                if (ImGui::MenuItem("Save As"))
                {
                    std::string path = SaveFileDialog();
                    if (!path.empty())
                    {
                        m_CurrentScenePath = path;
                        Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), path);
                    }
                }

                ImGui::Separator();

                if (
                    ImGui::MenuItem(
                        "Exit"
                    )
                    )
                {
                    Pondo::Application
                        ::Get()
                        .Close();
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Scene", nullptr, &m_ShowScene);
                ImGui::MenuItem("Properties", nullptr, &m_ShowProps);
                ImGui::MenuItem("Stats", nullptr, &m_ShowStats);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Cube"))   CreateEntity(0);
                if (ImGui::MenuItem("Sphere")) CreateEntity(1);
                if (ImGui::MenuItem("Plane"))  CreateEntity(2);
                if (ImGui::MenuItem("Cylinder")) CreateEntity(3);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        float menuH = ImGui::GetWindowHeight();
        ImGui::End();

        const float sideW = 260.0f;
        const float topY = menuH;
        const float contentH = (float)h - topY;
        float leftW = m_ShowScene ? sideW : 0.0f;
        float rightW = (m_ShowProps || m_ShowStats) ? sideW : 0.0f;

        // ── Scene panel ──────────────────────────────────────────────
        if (m_ShowScene) {
            ImGui::SetNextWindowPos({ 0, topY });
            ImGui::SetNextWindowSize({ sideW, contentH });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::Begin("Scene##panel", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::PopStyleVar();

            ImGui::Text("Scene: %s", m_SceneLayer->GetScene()->GetName().c_str());
            ImGui::Separator();
            for (auto& ep : m_SceneLayer->GetScene()->GetEntities()) {
                bool sel = (m_SceneLayer->GetSelectedEntity() == ep.get());
                ImGui::PushID((int)ep->GetID());
                if (ImGui::Selectable(ep->GetTag().c_str(), sel))
                    m_SceneLayer->SetSelectedEntity(ep.get());
                ImGui::PopID();
            }
            ImGui::Separator();
            if (ImGui::Button("+ Cube")) CreateEntity(0);
            ImGui::SameLine();
            if (ImGui::Button("+ Sphere")) CreateEntity(1);
            ImGui::SameLine();
            if (ImGui::Button("+ Plane")) CreateEntity(2);
            if (ImGui::Button("+ Cylinder")) CreateEntity(3);

            if (m_SceneLayer->GetSelectedEntity()) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.65f,0.15f,0.15f,1 });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.80f,0.20f,0.20f,1 });
                if (ImGui::Button("Delete Selected")) m_SceneLayer->DeleteSelected();
                ImGui::PopStyleColor(2);
            }
            ImGui::End();
        }

        // ── Properties panel ─────────────────────────────────────────
        if (m_ShowProps || m_ShowStats) {
            ImGui::SetNextWindowPos({ (float)w - sideW, topY });
            ImGui::SetNextWindowSize({ sideW, contentH });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::Begin("Properties##panel", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::PopStyleVar();

            if (m_ShowStats) {
                ImGui::Text("Pondo Engine");
                ImGui::Text("%.1f FPS  (%.3f ms)", io.Framerate, 1000.0f / io.Framerate);
                ImGui::Text("Entities: %zu", m_SceneLayer->GetScene()->GetEntities().size());
                if (auto* cam = m_SceneLayer->GetCamera()) {
                    glm::vec3 p = cam->GetPosition();
                    ImGui::Text("Cam: (%.1f, %.1f, %.1f)", p.x, p.y, p.z);
                    ImGui::Text("Yaw: %.1f  Pitch: %.1f", cam->GetYaw(), cam->GetPitch());
                }
                ImGui::Separator();
                ImGui::Text("Gizmo:");
                ImGui::Text("1 Move");
                ImGui::Text("2 Rotate");
                ImGui::Text("3 Scale");
            }

            if (m_ShowProps) {
                if (Pondo::Entity* sel = m_SceneLayer->GetSelectedEntity()) {
                    char buf[128]; strncpy_s(buf, sel->GetTag().c_str(), sizeof(buf));
                    if (ImGui::InputText("Name", buf, sizeof(buf))) sel->SetTag(buf);
                    ImGui::Separator();
                    ImGui::Text("Transform Controls");

                    ImGui::Separator();

                    ImGui::Checkbox("Enable Increment Snap", &m_EnableSnapping);

                    ImGui::InputFloat(
                        "Move Step",
                        &m_MoveIncrement,
                        0.0f,
                        0.0f,
                        "%.3f"
                    );

                    ImGui::InputFloat(
                        "Rotate Step",
                        &m_RotateIncrement,
                        0.0f,
                        0.0f,
                        "%.2f"
                    );

                    ImGui::InputFloat(
                        "Scale Step",
                        &m_ScaleIncrement,
                        0.0f,
                        0.0f,
                        "%.3f"
                    );

                    // prevent invalid values
                    m_MoveIncrement =
                        std::max(0.001f, m_MoveIncrement);

                    m_RotateIncrement =
                        std::max(0.01f, m_RotateIncrement);

                    m_ScaleIncrement =
                        std::max(0.001f, m_ScaleIncrement);

                    ImGui::Separator();


                    auto& tf = sel->GetTransform();

                    auto original = tf;

                    if (
                        ImGui::IsWindowFocused() &&
                        !m_WasEditingTransform
                        )
                    {
                        m_TransformBefore =
                            tf;
                    }

                    bool changed = false;

                    changed |= ImGui::InputFloat3(
                        "Position",
                        &tf.Position.x,
                        "%.3f"
                    );

                    changed |= ImGui::InputFloat3(
                        "Rotation",
                        &tf.Rotation.x,
                        "%.2f"
                    );

                    changed |= ImGui::InputFloat3(
                        "Scale",
                        &tf.Scale.x,
                        "%.3f"
                    );

                    tf.Scale.x =
                        std::max(
                            0.001f,
                            tf.Scale.x
                        );

                    tf.Scale.y =
                        std::max(
                            0.001f,
                            tf.Scale.y
                        );

                    tf.Scale.z =
                        std::max(
                            0.001f,
                            tf.Scale.z
                        );

                    if (
                        changed
                        )
                    {
                        m_WasEditingTransform =
                            true;
                    }

                    if (
                        m_WasEditingTransform &&
                        ImGui::IsMouseReleased(
                            ImGuiMouseButton_Left
                        )
                        )
                    {
                        if (
                            memcmp(
                                &m_TransformBefore,
                                &tf,
                                sizeof(tf)
                            ) != 0
                            )
                        {
                            Execute(
                                std::make_unique<
                                TransformCommand
                                >(
                                    sel,
                                    m_TransformBefore,
                                    tf
                                )
                            );
                        }

                        m_WasEditingTransform = false;
                    }

                    if (auto* mc = sel->GetMaterial(); mc && mc->Mat) {
                        ImGui::Separator(); ImGui::Text("Material");
                        glm::vec4 col = mc->Mat->GetColor();
                        float ca[4] = { col.r,col.g,col.b,col.a };
                        if (ImGui::ColorEdit4("Color", ca)) mc->Mat->SetColor({ ca[0],ca[1],ca[2],ca[3] });
                    }
                }
                else {
                    ImGui::TextDisabled("Nothing selected.");
                    ImGui::TextDisabled("Click an object in the viewport");
                    ImGui::TextDisabled("or the scene panel.");
                }
            }
            ImGui::End();
        }

        // ── Viewport ─────────────────────────────────────────────────
        float vpX = leftW;
        float vpW = (float)w - leftW - rightW;
        ImGui::SetNextWindowPos({ vpX, topY });
        ImGui::SetNextWindowSize({ vpW, contentH });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Viewport##panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar(2);

        // Viewport is focused when the mouse is over it AND ImGui doesn't
        // want to capture the mouse for its own panels
        bool vpHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        m_SceneLayer->SetViewportFocused(vpHovered);

        ImVec2 contentPos = ImGui::GetCursorScreenPos();
        ImVec2 panelSize = ImGui::GetContentRegionAvail();

        if (panelSize.x > 0 && panelSize.y > 0)
        {
            unsigned pw = (unsigned)panelSize.x;
            unsigned ph = (unsigned)panelSize.y;

            if (
                pw != m_Framebuffer->GetSpec().Width ||
                ph != m_Framebuffer->GetSpec().Height
                )
            {
                m_Framebuffer->Resize(pw, ph);

                Pondo::WindowResizeEvent ev(pw, ph);

                m_SceneLayer->OnEvent(ev);
            }

            m_SceneLayer->SetViewportRect(
                contentPos.x,
                contentPos.y,
                panelSize.x,
                panelSize.y
            );

            ImGui::Image(
                (ImTextureID)(uintptr_t)
                m_Framebuffer->GetColorAttachmentID(),
                panelSize,
                { 0,1 },
                { 1,0 }
            );

            glm::vec2 mouse =
            {
                io.MousePos.x,
                io.MousePos.y
            };

            bool overViewport =
                mouse.x >= contentPos.x &&
                mouse.x < contentPos.x + panelSize.x &&
                mouse.y >= contentPos.y &&
                mouse.y < contentPos.y + panelSize.y;

            bool lmbDown =
                io.MouseDown[0];

            bool lmbClicked =
                io.MouseClicked[0];

            bool rmbDown =
                io.MouseDown[1];

            bool dragging =
                m_SceneLayer->IsDraggingGizmo();

            // viewport focus
            m_SceneLayer->SetViewportFocused(
                overViewport &&
                !dragging
            );

            // Ctrl+B → Duplicate selected entity
            if (
                overViewport &&
                io.KeyCtrl &&
                ImGui::IsKeyPressed(ImGuiKey_B, false)
                )
            {
                Pondo::Entity* copy =
                    m_SceneLayer->DuplicateEntity(
                        m_SceneLayer->GetSelectedEntity()
                    );

                if (copy)
                {
                    m_SceneLayer
                        ->SetSelectedEntity(
                            copy
                        );
                }
            }

            // update active drag
            if (dragging)
            {
                if (lmbDown)
                {
                    m_SceneLayer->UpdateGizmoDrag(
                        mouse,
                        m_EnableSnapping,
                        m_MoveIncrement,
                        m_RotateIncrement,
                        m_ScaleIncrement
                    );
                }
                else
                {
                    if (
                        m_SceneLayer->EndGizmoDrag()
                        &&
                        m_SceneLayer->GetSelectedEntity()
                        )
                    {
                        Execute(
                            std::make_unique<TransformCommand>(
                                m_SceneLayer->GetSelectedEntity(),
                                m_TransformBefore,
                                m_SceneLayer
                                ->GetSelectedEntity()
                                ->GetTransform()
                            )
                        );
                    }
                }
            }

            // start drag or pick
            else if (
                overViewport &&
                lmbClicked &&
                !rmbDown
                )
            {
                int axis =
                    m_SceneLayer->GizmoAxisHit(
                        mouse
                    );

                if (axis >= 0)
                {
                    m_TransformBefore =
                        m_SceneLayer
                        ->GetSelectedEntity()
                        ->GetTransform();

                    m_SceneLayer->BeginGizmoDrag(
                        axis,
                        mouse
                    );
                }
                else
                {
                    m_SceneLayer->TryPickEntity(
                        mouse
                    );
                }
            }
        }

        if (m_StatusTimer > 0.0f)
        {
            m_StatusTimer -= io.DeltaTime;

            ImGui::SetNextWindowBgAlpha(0.8f);

            ImGui::SetNextWindowPos(
                {
                    (float)w * 0.5f - 60,
                    40
                }
            );

            ImGui::Begin(
                "##status",
                nullptr,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove
            );

            ImGui::Text(
                "%s",
                m_StatusText.c_str()
            );

            ImGui::End();
        }

        ImGui::End();

        // ── Flush ─────────────────────────────────────────────────────
        glDisable(GL_DEPTH_TEST);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glEnable(GL_DEPTH_TEST);
    }

    void Execute(
        std::unique_ptr<ICommand> cmd
    )
    {
        while (
            (int)m_History.size()
                >
            m_HistoryIndex + 1
            )
        {
            m_History.pop_back();
        }

        cmd->Redo();

        m_History.push_back(
            std::move(cmd)
        );

        m_HistoryIndex++;
    }

    void Undo()
    {
        if (m_HistoryIndex < 0)
            return;

        m_History[
            m_HistoryIndex
        ]->Undo();

        m_HistoryIndex--;

        m_StatusText =
            "Undo";

        m_StatusTimer =
            1.0f;
    }

    void Redo()
    {
        if (
            m_HistoryIndex + 1
            >=
            (int)m_History.size()
            )
            return;

        m_HistoryIndex++;

        m_History[
            m_HistoryIndex
        ]->Redo();

        m_StatusText =
            "Redo";

        m_StatusTimer =
            1.0f;
    }

    std::string m_StatusText;
    float m_StatusTimer = 0.0f;

private:
    void CreateEntity(int meshType)
    {
        static int count = 0;

        const char* names[] =
        {
            "Cube",
            "Sphere",
            "Plane",
            "Cylinder"
        };

        glm::vec4 colors[] =
        {
            {0.8f,0.35f,0.2f,1},
            {0.2f,0.55f,0.85f,1},
            {0.55f,0.55f,0.55f,1},
            {0.75f,0.65f,0.25f,1}
        };

        std::shared_ptr<Pondo::Mesh> mesh;

        switch (meshType)
        {
        case 1:
            mesh =
                Pondo::Mesh::CreateSphere();
            break;

        case 2:
            mesh =
                Pondo::Mesh::CreatePlane(1.0f);
            break;

        case 3:
            mesh =
                Pondo::Mesh::CreateCylinder();
            break;

        default:
            mesh =
                Pondo::Mesh::CreateCube();
        }

        auto* e =
            m_SceneLayer->SpawnEntity(
                std::string(names[meshType]) +
                std::to_string(++count),
                std::move(mesh),
                colors[meshType],
                { 0,0.5f,0 }
            );

        m_SceneLayer->SetSelectedEntity(e);
    }

    GLFWwindow* m_Window = nullptr;

    std::shared_ptr<Pondo::Framebuffer> m_Framebuffer;

    SceneLayer* m_SceneLayer = nullptr;

    float m_LastTime = 0.0f;

    std::string m_CurrentScenePath;

    bool m_ShowScene = true;
    bool m_ShowProps = true;
    bool m_ShowStats = true;

    bool m_EnableSnapping = false;

    float m_MoveIncrement = 0.5f;
    float m_RotateIncrement = 15.0f;
    float m_ScaleIncrement = 0.1f;



    std::vector<
        std::unique_ptr<ICommand>
    > m_History;

    int m_HistoryIndex = -1;

    bool m_WasEditingTransform = false;

    std::remove_reference_t<decltype(std::declval<Pondo::Entity>().GetTransform())> m_TransformBefore{};
};

// -------------------------------------------------------
//  Application
// -------------------------------------------------------

class Sandbox : public Pondo::Application {
public:
    Sandbox() {
        Pondo::Application::InitGlad();
        m_SceneLayer = new SceneLayer();
        m_EditorLayer = new EditorLayer(m_SceneLayer);
        PushLayer(m_SceneLayer);
        PushOverlay(m_EditorLayer);
    }
    ~Sandbox() { delete m_SceneLayer; delete m_EditorLayer; }
private:
    SceneLayer* m_SceneLayer = nullptr;
    EditorLayer* m_EditorLayer = nullptr;
};

Pondo::Application* Pondo::CreateApplication() { return new Sandbox(); }
