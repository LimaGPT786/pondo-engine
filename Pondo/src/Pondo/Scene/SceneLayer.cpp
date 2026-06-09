#include "SceneLayer.h"
#include "Pondo/Renderer/RayUtils.h"

#include <Pondo.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Pondo/Scene/SceneSerializer.h"

// -------------------------------------------------------
//  Construction / wiring
// -------------------------------------------------------

SceneLayer::SceneLayer() : Layer("SceneLayer") {}
static bool s_prevLmb = false;
void SceneLayer::SetFramebuffer(std::shared_ptr<Pondo::Framebuffer> fb)
{
    m_Framebuffer = fb;
}

void SceneLayer::SetViewportFocused(bool v) { m_ViewportFocused = v; }

void SceneLayer::SetViewportRect(float x, float y, float w, float h)
{
    m_VpPos = { x, y };
    m_VpSize = { w, h };
}

void SceneLayer::SetSnapSettings(bool enabled, float move, float rotate, float scale)
{
    m_EnableSnapping = enabled;
    m_MoveIncrement = move;
    m_RotateIncrement = rotate;
    m_ScaleIncrement = scale;
}

// -------------------------------------------------------
//  Scene operations
// -------------------------------------------------------

void SceneLayer::LoadScene(const std::string& path)
{
    m_Selection.clear();
    m_Scene->Clear();
    Pondo::SceneSerializer::Load(m_Scene.get(), path, m_Shader);

    auto& entities = m_Scene->GetEntities();
    if (!entities.empty())
        m_Selection.push_back(entities[0].get());
}

void SceneLayer::CollectLights()
{
    auto ambient = m_Lights.AmbientColor;
    auto intensity = m_Lights.AmbientIntensity;
    auto sun = m_Lights.Sun;

    m_Lights.Clear();

    m_Lights.AmbientColor = ambient;
    m_Lights.AmbientIntensity = intensity;
    m_Lights.Sun = sun;

    for (const auto& ep : m_Scene->GetEntities())
    {
        auto* e = ep.get();

        if (!e->HasLight())
            continue;

        auto* lc = e->GetLight();

        if (!lc)
            continue;

        const auto& tf =
            e->GetTransform();

        if (lc->Type == Pondo::LightType::Point)
        {
            if (m_Lights.PointLightCount >=
                Pondo::MAX_POINT_LIGHTS)
                continue;

            auto& out =
                m_Lights.PointLights[
                    m_Lights.PointLightCount++
                ];

            out.Position =
                tf.Position;

            if (glm::length(lc->Color) < 0.01f)
                out.Color = { 1.0f, 1.0f, 1.0f };
            else
                out.Color = lc->Color;

            out.Intensity =
                glm::max(
                    0.0f,
                    lc->Intensity);

            out.Constant =
                1.0f;

            out.Linear =
                0.01f;

            out.Quadratic =
                0.0005f;

            out.Range = lc->Range;
        }

        if (
            lc->Type
            ==
            Pondo::LightType::Spot)
        {
            if (
                m_Lights.SpotLightCount
                >=
                Pondo::MAX_SPOT_LIGHTS)
            {
                continue;
            }

            auto& out =
                m_Lights
                .
                SpotLights[
                    m_Lights
                        .
                        SpotLightCount++
                ];

            out.Position =
                tf.Position;

            glm::vec3 dir =
                lc
                ->
                Direction;

            if (
                glm::length(
                    dir)
                <
                0.001f)
            {
                dir =
                {
                    0,
                    -1,
                    0
                };
            }

            out.Direction =
                glm::normalize(
                    dir);

            out.Color =
                glm::length(
                    lc
                    ->
                    Color)
                <
                0.01f
                ?
                glm::vec3(
                    1)
                :
                lc
                ->
                Color;

            out.Intensity =
                glm::max(
                    0.0f,
                    lc
                    ->
                    Intensity);

            out.Constant =
                glm::max(
                    0.001f,
                    lc
                    ->
                    Constant);

            out.Linear =
                glm::max(
                    0.0f,
                    lc
                    ->
                    Linear);

            out.Quadratic =
                glm::max(
                    0.0f,
                    lc
                    ->
                    Quadratic);

            out.InnerCutoffCos =
                glm::cos(
                    glm::radians(
                        glm::clamp(
                            lc->InnerCutoffDeg,
                            0.1f,
                            89.9f)));

            out.OuterCutoffCos =
                glm::cos(
                    glm::radians(
                        glm::clamp(
                            lc->OuterCutoffDeg,
                            lc->InnerCutoffDeg + 0.1f,
                            89.9f)));
        }
    }
}

Pondo::Entity* SceneLayer::SpawnEntity(const std::string& name,
    std::shared_ptr<Pondo::Mesh> mesh,
    glm::vec4 color,
    glm::vec3 pos)
{
    auto* e = m_Scene->CreateEntity(name);
    e->GetTransform().Position = pos;
    e->SetMesh(mesh);
    e->SetMaterial(std::make_shared<Pondo::Material>(m_Shader));
    e->GetMaterial()->Mat->SetColor(color);
    return e;
}

Pondo::Entity* SceneLayer::SpawnLight(
    const std::string& name,
    Pondo::LightType type,
    glm::vec3 pos)
{
    auto* e =
        m_Scene
        ->
        CreateEntity(
            name);

    e
        ->
        GetTransform()
        .
        Position =
        pos;

    e
        ->
        AddLight(
            type);

    if (
        !e
        ->
        HasLight())
    {
        return e;
    }

    auto* lc =
        e
        ->
        GetLight();

    if (
        !lc)
    {
        return e;
    }

    lc->Type =
        type;

    lc->Color =
    {
        1,
        1,
        1
    };

    lc->Intensity =
        2.0f;

    lc->Range =
        25.0f;

    lc->Constant =
        1.0f;

    lc->Linear =
        0.045f;

    lc->Quadratic =
        0.0075f;

    lc->Direction =
    {
        0,
        -1,
        0
    };

    return e;
}

Pondo::Entity* SceneLayer::DuplicateEntity(Pondo::Entity* src)
{
    if (!src) return nullptr;

    // Light-only entities have no mesh — handle them separately.
    auto* srcMesh = src->GetMesh();
    if (!srcMesh || !srcMesh->MeshData)
    {
        auto* copy = m_Scene->CreateEntity(src->GetTag() + "_Copy");
        copy->GetTransform() = src->GetTransform();
        if (src->HasLight())
        {
            auto* lc = src->GetLight();
            copy->AddLight(lc->Type);
            if (auto* clc = copy->GetLight())
                *clc = *lc;
        }
        return copy;
    }

    glm::vec4 color = { 1, 1, 1, 1 };
    if (src->GetMaterial() && src->GetMaterial()->Mat)
        color = src->GetMaterial()->Mat->GetColor();

    auto* copy = SpawnEntity(
        src->GetTag() + "_Copy",
        srcMesh->MeshData,
        color,
        src->GetTransform().Position);

    copy->GetTransform() = src->GetTransform();
    return copy;
}

void SceneLayer::DeleteSelected()
{
    if (m_Selection.empty()) return;

    // Collect IDs first so the vector stays valid during erasure.
    std::vector<uint32_t> ids;
    ids.reserve(m_Selection.size());
    for (auto* e : m_Selection) ids.push_back(e->GetID());

    m_Selection.clear();
    for (uint32_t id : ids)
        m_Scene->DestroyEntity(id);
}

// -------------------------------------------------------
//  Picking
// -------------------------------------------------------

void SceneLayer::TryPickEntity(
    glm::vec2 px,
    bool addToSelection)
{
    if (
        m_VpSize.x <= 0 ||
        m_VpSize.y <= 0)
    {
        return;
    }

    Ray ray =
        ScreenToRay(
            px,
            m_VpPos,
            m_VpSize,
            glm::inverse(
                m_Camera
                ->
                GetViewProjection()));

    Pondo::Entity*
        best =
        nullptr;

    float bestT =
        1e30f;

    for (
        auto& ep :
        m_Scene
        ->
        GetEntities())
    {
        auto* e =
            ep.get();

        if (
            !e
            ->
            GetMesh())
        {
            continue;
        }

        glm::vec3 c =
            e
            ->
            GetTransform()
            .
            Position;

        glm::vec3 h =
            glm::abs(
                e
                ->
                GetTransform()
                .
                Scale)
            *
            0.5f;

        float t =
            RayAABB(
                ray,
                c - h,
                c + h);

        if (
            t > 0 &&
            t < bestT)
        {
            best =
                e;

            bestT =
                t;
        }
    }

    if (
        !best)
    {
        if (
            !addToSelection)
        {
            m_Selection.clear();
        }

        return;
    }

    if (
        addToSelection)
    {
        bool found =
            false;

        for (
            auto* e :
            m_Selection)
        {
            if (
                e ==
                best)
            {
                found =
                    true;

                break;
            }
        }

        if (
            !found)
        {
            m_Selection
                .
                push_back(
                    best);
        }

        return;
    }

    m_Selection.clear();

    m_Selection.push_back(
        best);
}

// -------------------------------------------------------
//  Gizmo hit-testing
// -------------------------------------------------------

int SceneLayer::GizmoAxisHit(glm::vec2 px)
{
    if (m_Selection.empty()) return -1;

    // Use the same averaged origin that DrawGizmo uses so hit-testing
    // matches what's actually drawn when multiple entities are selected.
    glm::vec3 origin(0);
    for (auto* e : m_Selection)
        origin += e->GetTransform().Position;
    origin /= (float)m_Selection.size();

    float gizmoScale = glm::length(m_Camera->GetPosition() - origin) * 0.15f;

    // --- ROTATION MODE ---
    if (m_GizmoMode == 1)
    {
        int   bestAxis = -1;
        float bestDist = 999999.0f;

        for (int axis = 0; axis < 3; axis++)
        {
            const int segments = 64;
            for (int i = 0; i < segments; i++)
            {
                float a0 = glm::two_pi<float>() * ((float)i / segments);
                float a1 = glm::two_pi<float>() * ((float)(i + 1) / segments);

                glm::vec3 p0, p1;
                if (axis == 0) {
                    p0 = origin + glm::vec3(0, cos(a0), sin(a0)) * gizmoScale;
                    p1 = origin + glm::vec3(0, cos(a1), sin(a1)) * gizmoScale;
                }
                else if (axis == 1) {
                    p0 = origin + glm::vec3(cos(a0), 0, sin(a0)) * gizmoScale;
                    p1 = origin + glm::vec3(cos(a1), 0, sin(a1)) * gizmoScale;
                }
                else {
                    p0 = origin + glm::vec3(cos(a0), sin(a0), 0) * gizmoScale;
                    p1 = origin + glm::vec3(cos(a1), sin(a1), 0) * gizmoScale;
                }

                glm::vec2 s0 = WorldToScreen(p0, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
                glm::vec2 s1 = WorldToScreen(p1, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
                glm::vec2 seg = s1 - s0;
                float     len = glm::dot(seg, seg);
                if (len < 1.0f) continue;

                float     t = glm::clamp(glm::dot(px - s0, seg) / len, 0.0f, 1.0f);
                glm::vec2 hit = s0 + seg * t;
                float     dist = glm::length(px - hit);

                if (dist < bestDist) { bestDist = dist; bestAxis = axis; }
            }
        }
        return bestDist < 75.0f ? bestAxis : -1;
    }

    // --- MOVE + SCALE MODE ---
    glm::vec3 axes[3] =
    {
        {1,0,0},
        {0,1,0},
        {0,0,1}
    };

    if (
        m_GizmoSpace
        ==
        1)
    {
        glm::mat4 rot =
            glm::eulerAngleXYZ(
                glm::radians(
                    m_Selection[0]
                    ->
                    GetTransform()
                    .
                    Rotation
                    .
                    x),

                glm::radians(
                    m_Selection[0]
                    ->
                    GetTransform()
                    .
                    Rotation
                    .
                    y),

                glm::radians(
                    m_Selection[0]
                    ->
                    GetTransform()
                    .
                    Rotation
                    .
                    z));

        for (
            int i = 0;
            i < 3;
            i++)
        {
            axes[i] =
                glm::normalize(
                    glm::vec3(
                        rot
                        *
                        glm::vec4(
                            axes[i],
                            0)));
        }
    }

    glm::vec3 tips[3];

    for (
        int i = 0;
        i < 3;
        i++)
    {
        tips[i] =
            origin
            +
            axes[i]
            *
            gizmoScale;
    }

    int   bestAxis = -1;
    float bestDist = 999999.0f;

    for (int i = 0; i < 3; i++)
    {
        glm::vec2 a = WorldToScreen(origin, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
        glm::vec2 b = WorldToScreen(tips[i], m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
        glm::vec2 ab = b - a;
        float     len = glm::dot(ab, ab);
        if (len < 1.0f) continue;

        float     t = glm::clamp(glm::dot(px - a, ab) / len, 0.0f, 1.0f);
        glm::vec2 hit = a + ab * t;
        float     dist = glm::length(px - hit);
        if (dist < bestDist) { bestDist = dist; bestAxis = i; }
    }
    return bestDist < 70.0f ? bestAxis : -1;
}

// -------------------------------------------------------
//  Gizmo drag
// -------------------------------------------------------

void SceneLayer::BeginGizmoDrag(int axis, glm::vec2 px)
{
    if (m_Selection.empty()) return;
    m_DragAxis = axis;
    m_DragStartPx = px;

    // Use averaged center as the gizmo anchor (matches DrawGizmo + GizmoAxisHit).
    glm::vec3 center(0);
    for (auto* e : m_Selection)
        center += e->GetTransform().Position;
    center /= (float)m_Selection.size();
    m_DragStartPos = center;

    auto& tf = m_Selection[0]->GetTransform();
    m_DragStartScale = tf.Scale;
    m_DragStartRot = tf.Rotation;

    // Record starting transforms for secondary selected entities.
    m_DragStartOffsets.clear();
    m_DragStartScales.clear();
    m_DragStartOffsetRots.clear();
    for (size_t i = 1; i < m_Selection.size(); ++i)
    {
        auto& stf = m_Selection[i]->GetTransform();
        m_DragStartOffsets.push_back(stf.Position);
        m_DragStartScales.push_back(stf.Scale);
        m_DragStartOffsetRots.push_back(stf.Rotation);
    }

    // Also record primary entity start position for move mode.
    m_DragStartOffsets.insert(m_DragStartOffsets.begin(), tf.Position);
    m_DragStartScales.insert(m_DragStartScales.begin(), tf.Scale);
    m_DragStartOffsetRots.insert(m_DragStartOffsetRots.begin(), tf.Rotation);
}

void SceneLayer::UpdateGizmoDrag(glm::vec2 px, bool enableSnap,
    float moveIncrement, float rotateIncrement, float scaleIncrement)
{
    if (m_DragAxis < 0 || m_Selection.empty()) return;

    Pondo::Entity* primary = m_Selection[0];
    auto& primaryTf = primary->GetTransform();

    // Determine the drag axis direction in world space.
    // In Local space, rotate the axis by the primary entity's orientation.
    static const glm::vec3 worldAxes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    glm::vec3 axis = worldAxes[m_DragAxis];

    if (m_GizmoSpace == 1) // Local
    {
        // Build rotation matrix from primary entity's Euler angles.
        glm::mat4 rotMat = glm::eulerAngleXYZ(
            glm::radians(primaryTf.Rotation.x),
            glm::radians(primaryTf.Rotation.y),
            glm::radians(primaryTf.Rotation.z));
        axis = glm::normalize(glm::vec3(rotMat * glm::vec4(axis, 0.0f)));
    }

    // Project drag delta onto screen-space axis direction.
    glm::vec2 p0 = WorldToScreen(m_DragStartPos, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
    glm::vec2 p1 = WorldToScreen(m_DragStartPos + axis, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
    glm::vec2 dir = p1 - p0;
    float     len = glm::length(dir);
    if (len < 1.0f) return;
    dir /= len;

    float dragAmount = glm::dot(px - m_DragStartPx, dir) * 0.01f;

    switch (m_GizmoMode)
    {
    case 0: // Move — apply delta to ALL selected entities from their recorded starts
    {
        if (enableSnap && moveIncrement > 0.0f)
            dragAmount = round(dragAmount / moveIncrement) * moveIncrement;

        glm::vec3 delta = axis * dragAmount;

        for (size_t i = 0; i < m_Selection.size(); i++)
        {
            m_Selection[i]->GetTransform().Position =
                m_DragStartOffsets[i] + delta;
        }

        break;
    }
    case 1: // Rotate — same angle applied to all
    {
        float rotation = dragAmount * 100.0f;
        if (enableSnap && rotateIncrement > 0.0f)
            rotation = round(rotation / rotateIncrement) * rotateIncrement;

        for (size_t i = 0; i < m_Selection.size(); ++i)
        {
            auto& tf = m_Selection[i]->GetTransform();
            tf.Rotation[m_DragAxis] = m_DragStartOffsetRots[i][m_DragAxis] + rotation;
        }
        break;
    }
    case 2: // Scale — each entity scaled around its own pivot
    {
        if (enableSnap && scaleIncrement > 0.0f)
            dragAmount = round(dragAmount / scaleIncrement) * scaleIncrement;

        primaryTf.Scale[m_DragAxis] =
            glm::max(0.001f, float(m_DragStartScales[0][m_DragAxis] + dragAmount));

        for (size_t i = 1; i < m_Selection.size(); ++i)
        {
            auto& tf = m_Selection[i]->GetTransform();
            tf.Scale[m_DragAxis] =
                glm::max(0.001f, float(m_DragStartScales[i][m_DragAxis] + dragAmount));
        }
        break;
    }
    }
}

bool SceneLayer::EndGizmoDrag()
{
    bool dragged = m_DragAxis >= 0;
    m_DragAxis = -1;
    return dragged;
}

// -------------------------------------------------------
//  Layer lifecycle
// -------------------------------------------------------

void SceneLayer::OnAttach()
{
    m_Camera = std::make_shared<Pondo::Camera>(60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
    m_Shader = std::make_shared<Pondo::Shader>(s_VertSrc, s_FragSrc);
    m_FlatShader = std::make_shared<Pondo::Shader>(s_FlatVertSrc, s_FlatFragSrc);
    m_Scene = std::make_unique<Pondo::Scene>("Main Scene");

    SpawnEntity("Cube", Pondo::Mesh::CreateCube(), { 0.8f, 0.35f, 0.2f,  1 }, { 0,    0.5f, 0 });
    SpawnEntity("Sphere", Pondo::Mesh::CreateSphere(), { 0.2f, 0.55f, 0.85f, 1 }, { 2.5f, 0.5f, 0 });

    auto* ground = m_Scene->CreateEntity("Ground");
    ground->GetTransform().Scale = { 20, 1, 20 };
    ground->GetTransform().Position = { 0, -0.5f, 0 };
    ground->SetMesh(Pondo::Mesh::CreatePlane(1.0f));
    ground->SetMaterial(std::make_shared<Pondo::Material>(m_Shader));
    ground->GetMaterial()->Mat->SetColor({ 0.3f, 0.5f, 0.3f, 1 });
}

void SceneLayer::OnUpdate(Pondo::Timestep ts)
{
    if (!m_ViewportFocused) { m_FirstMouseLook = true; return; }

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
    else {
        m_FirstMouseLook = true;
    }

    if (IsDraggingGizmo())
    {
        auto [mx, my] =
            Pondo::Input
            ::GetMousePosition();

        UpdateGizmoDrag(
            {
                mx,
                my
            },
            m_EnableSnapping,
            m_MoveIncrement,
            m_RotateIncrement,
            m_ScaleIncrement);
    }

    bool lmb =
        Pondo::Input
        ::IsMouseButtonPressed(
            GLFW_MOUSE_BUTTON_LEFT);

    auto [mx, my] =
        Pondo::Input
        ::GetMousePosition();

    glm::vec2 mouse =
    {
        mx,
        my
    };

    bool pressed =
        lmb
        &&
        !s_prevLmb;

    bool released =
        !lmb
        &&
        s_prevLmb;

    if (
        pressed)
    {
        int axis =
            GizmoAxisHit(
                mouse);

        if (
            axis >= 0)
        {
            BeginGizmoDrag(
                axis,
                mouse);
        }
        else
        {
            bool add =
                Pondo::Input
                ::IsKeyPressed(
                    GLFW_KEY_LEFT_SHIFT)
                ||
                Pondo::Input
                ::IsKeyPressed(
                    GLFW_KEY_RIGHT_SHIFT);

            TryPickEntity(
                mouse,
                add);
        }
    }

    if (
        lmb
        &&
        IsDraggingGizmo())
    {
        UpdateGizmoDrag(
            mouse,
            m_EnableSnapping,
            m_MoveIncrement,
            m_RotateIncrement,
            m_ScaleIncrement);
    }

    if (
        released)
    {
        EndGizmoDrag();
    }

    s_prevLmb =
        lmb;
}

void SceneLayer::OnRender()
{
    if (!m_Framebuffer) return;

    if (!m_Initialized) {
        m_Grid.Build(50, 1);
        m_Arrow.Build();
        m_Initialized = true;
    }

    m_Framebuffer->Bind();
    Pondo::Renderer::SetClearColor(0.12f, 0.12f, 0.13f, 1.0f);
    Pondo::Renderer::Clear();

    CollectLights();

    Pondo::Renderer::BeginScene(
        *m_Camera,
        m_Lights
    );

    glEnable(GL_DEPTH_TEST);
    for (auto& ep : m_Scene->GetEntities()) {
        auto* mc = ep->GetMesh();
        auto* mat = ep->GetMaterial();
        if (!mc || !mat || !mc->MeshData || !mat->Mat) continue;
        Pondo::Renderer::SubmitMesh(mc->MeshData, mat->Mat, ep->GetTransform().GetTransform());
    }

    m_FlatShader->Bind();
    m_FlatShader->SetMat4("u_ViewProjection", m_Camera->GetViewProjection());
    m_FlatShader->SetMat4("u_Transform", glm::mat4(1.0f));
    m_FlatShader->SetFloat4("u_Color", { 0.35f, 0.35f, 0.38f, 1 });
    glLineWidth(1.0f);
    m_Grid.Draw();

    if (!m_Selection.empty()) {
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        m_FlatShader->Bind();
        m_FlatShader->SetMat4("u_ViewProjection", m_Camera->GetViewProjection());

        for (size_t i = 0; i < m_Selection.size(); ++i)
        {
            auto* ent = m_Selection[i];
            auto& tf = ent->GetTransform();
            glm::mat4 transform = tf.GetTransform() * glm::scale(glm::mat4(1), glm::vec3(1.05f));
            m_FlatShader->SetMat4("u_Transform", transform);

            // Primary = gold, secondary selections = cyan
            if (i == 0)
                m_FlatShader->SetFloat4("u_Color", { 1, 0.85f, 0.1f, 1 });
            else
                m_FlatShader->SetFloat4("u_Color", { 0.2f, 0.9f, 0.9f, 1 });

            auto* mc = ent->GetMesh();
            if (mc && mc->MeshData) {
                mc->MeshData->Bind();
                glDrawElements(GL_TRIANGLES, mc->MeshData->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
                mc->MeshData->Unbind();
            }
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        DrawGizmo();
        glEnable(GL_DEPTH_TEST);
    }

    Pondo::Renderer::EndScene();
    m_Framebuffer->Unbind();
}

void SceneLayer::OnEvent(Pondo::Event& e)
{
    Pondo::EventDispatcher d(e);

    d.Dispatch<Pondo::WindowResizeEvent>(
        [this](Pondo::WindowResizeEvent& ev)
        {
            if (
                ev.GetWidth() > 0 &&
                ev.GetHeight() > 0)
            {
                m_Camera
                    ->SetAspectRatio(
                        (float)
                        ev.GetWidth()
                        /
                        ev.GetHeight());
            }

            return false;
        });
}

// -------------------------------------------------------
//  Private gizmo drawing
// -------------------------------------------------------

void SceneLayer::DrawGizmo()
{
    if (m_Selection.empty()) return;

    if (Pondo::Input::IsKeyPressed(GLFW_KEY_1)) m_GizmoMode = 0;
    if (Pondo::Input::IsKeyPressed(GLFW_KEY_2)) m_GizmoMode = 1;
    if (Pondo::Input::IsKeyPressed(GLFW_KEY_3)) m_GizmoMode = 2;
    static bool gWasDown =
        false;

    bool gDown =
        Pondo::Input
        ::IsKeyPressed(
            GLFW_KEY_G);

    if (
        gDown
        &&
        !gWasDown)
    {
        m_GizmoSpace =
            1
            -
            m_GizmoSpace;

        m_GizmoHintTimer =
            1.0f;
    }

    gWasDown =
        gDown;

    glm::vec3 origin(0);

    for (
        auto* e :
        m_Selection)
    {
        origin +=
            e
            ->
            GetTransform()
            .
            Position;
    }

    origin /=
        (float)
        m_Selection
        .size();
    float     scale = glm::length(m_Camera->GetPosition() - origin) * 0.15f;

    // In Local space, build per-axis directions from the primary entity's rotation.
    glm::vec3 axisX = { 1, 0, 0 };
    glm::vec3 axisY = { 0, 1, 0 };
    glm::vec3 axisZ = { 0, 0, 1 };

    if (m_GizmoSpace == 1) // Local
    {
        glm::mat4 rotMat = glm::eulerAngleXYZ(
            glm::radians(m_Selection[0]->GetTransform().Rotation.x),
            glm::radians(m_Selection[0]->GetTransform().Rotation.y),
            glm::radians(m_Selection[0]->GetTransform().Rotation.z));
        axisX = glm::normalize(glm::vec3(rotMat * glm::vec4(axisX, 0)));
        axisY = glm::normalize(glm::vec3(rotMat * glm::vec4(axisY, 0)));
        axisZ = glm::normalize(glm::vec3(rotMat * glm::vec4(axisZ, 0)));
    }

    glDisable(GL_DEPTH_TEST);

    static float modeTimer =
        0.0f;

    static int last =
        0;

    if (
        last
        !=
        m_GizmoSpace)
    {
        modeTimer =
            1.5f;

        last =
            m_GizmoSpace;
    }

    if (
        modeTimer
    >
        0)
    {
        modeTimer
            -=
            0.016f;

        m_FlatShader
            ->
            Bind();

        glm::mat4 t =
            glm::translate(
                glm::mat4(1),
                origin
                +
                glm::vec3(
                    0,
                    scale
                    *
                    1.3f,
                    0));

        t =
            glm::scale(
                t,
                glm::vec3(
                    scale
                    *
                    0.15f));

        m_FlatShader
            ->
            SetMat4(
                "u_Transform",
                t);

        if (
            m_GizmoSpace
            ==
            0)
        {
            m_FlatShader
                ->
                SetFloat4(
                    "u_Color",
                    {
                        1,
                        0.8f,
                        0.2f,
                        1
                    });
        }
        else
        {
            m_FlatShader
                ->
                SetFloat4(
                    "u_Color",
                    {
                        0.3f,
                        1,
                        1,
                        1
                    });
        }

        glPointSize(
            20);

        m_Arrow
            .
            VA
            ->
            Bind();

        glDrawArrays(
            GL_POINTS,
            0,
            1);

        m_Arrow
            .
            VA
            ->
            Unbind();
    }

    struct Axis { glm::vec3 dir; glm::vec4 color; glm::vec4 active; };
    Axis axes[3] = {
        { axisX, {1,0.2f,0.2f,1}, {1,0.6f,0.6f,1} },
        { axisY, {0.2f,1,0.2f,1}, {0.6f,1,0.6f,1} },
        { axisZ, {0.2f,0.4f,1,1}, {0.6f,0.8f,1,1} }
    };

    for (int i = 0; i < 3; i++)
    {
        glm::vec4 col = (m_DragAxis == i) ? axes[i].active : axes[i].color;
        glm::vec3 tip = origin + axes[i].dir * scale;

        if (m_GizmoMode == 0) DrawArrow(origin, axes[i].dir, scale, col);
        else if (m_GizmoMode == 1) DrawRotationCircle(origin, axes[i].dir, scale, col);
        else                       DrawScaleDot(tip, scale, col);
    }

    glEnable(GL_DEPTH_TEST);
}

void SceneLayer::DrawArrow(const glm::vec3& origin, const glm::vec3& axis,
    float scale, const glm::vec4& color)
{
    glm::vec3 up = { 0, 1, 0 };
    glm::mat4 rot = glm::mat4(1);
    float d = glm::dot(up, axis);
    if (d < -0.9999f) rot = glm::rotate(glm::mat4(1), glm::pi<float>(), glm::vec3(1, 0, 0));
    else if (d < 0.9999f) rot = glm::rotate(glm::mat4(1), glm::acos(glm::clamp(d, -1.f, 1.f)),
        glm::normalize(glm::cross(up, axis)));
    glm::mat4 t = glm::translate(glm::mat4(1), origin) * rot * glm::scale(glm::mat4(1), glm::vec3(scale));
    m_FlatShader->SetMat4("u_Transform", t);
    m_FlatShader->SetFloat4("u_Color", color);
    m_Arrow.Draw();
}

void SceneLayer::DrawRotationCircle(
    const glm::vec3& origin,
    const glm::vec3& axis,
    float scale,
    const glm::vec4& color)
{
    constexpr int SEGMENTS = 64;

    std::vector<float>
        verts;

    float radius =
        scale *
        0.9f;

    glm::vec3 n =
        glm::normalize(
            axis);

    glm::vec3 helper =
        fabs(n.y)
        <
        0.99f
        ?
        glm::vec3(
            0,
            1,
            0)
        :
        glm::vec3(
            1,
            0,
            0);

    glm::vec3 right =
        glm::normalize(
            glm::cross(
                helper,
                n));

    glm::vec3 up =
        glm::normalize(
            glm::cross(
                n,
                right));

    for (
        int i = 0;
        i <= SEGMENTS;
        i++)
    {
        float a =
            glm::two_pi<float>()
            *
            (
                float(i)
                /
                SEGMENTS);

        glm::vec3 p =
            origin
            +
            right
            *
            cos(a)
            *
            radius
            +
            up
            *
            sin(a)
            *
            radius;

        verts.push_back(
            p.x);

        verts.push_back(
            p.y);

        verts.push_back(
            p.z);
    }

    auto vb =
        std::make_shared
        <
        Pondo
        ::
        VertexBuffer
        >
        (
            verts.data(),
            verts.size()
            *
            sizeof(float));

    auto va =
        std::make_shared
        <
        Pondo
        ::
        VertexArray
        >
        ();

    va
        ->
        AddVertexBuffer(
            vb);

    m_FlatShader
        ->
        Bind();

    m_FlatShader
        ->
        SetMat4(
            "u_Transform",
            glm::mat4(
                1));

    m_FlatShader
        ->
        SetFloat4(
            "u_Color",
            color);

    va
        ->
        Bind();

    glLineWidth(
        4);

    glDrawArrays(
        GL_LINE_STRIP,
        0,
        SEGMENTS +
        1);

    va
        ->
        Unbind();

    glLineWidth(
        1);
}

void SceneLayer::DrawScaleDot(const glm::vec3& pos, float size, const glm::vec4& color)
{
    m_FlatShader->Bind();
    glm::mat4 transform =
        glm::translate(glm::mat4(1.0f), pos) *
        glm::scale(glm::mat4(1.0f), glm::vec3(size * 0.08f));
    m_FlatShader->SetMat4("u_Transform", transform);
    m_FlatShader->SetFloat4("u_Color", color);
    glPointSize(18.0f);
    m_Arrow.VA->Bind();
    glDrawArrays(GL_POINTS, 0, 1);
    m_Arrow.VA->Unbind();
    glPointSize(1.0f);
}