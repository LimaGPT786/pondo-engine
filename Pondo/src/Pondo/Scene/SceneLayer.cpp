#include "SceneLayer.h"
#include "Pondo/Renderer/RayUtils.h"

#include <Pondo.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Pondo/Scene/SceneSerializer.h"
#include "Pondo/Scene/Entity.h"

#include "Pondo/Scripting/ScriptEngine.h"

extern "C" {
    #include <mcut/mcut.h>
}

// -------------------------------------------------------
//  Play lifecycle
// -------------------------------------------------------
void SceneLayer::TakeSnapshot()
{
    m_SceneSnapshot.clear();
    for (auto& ep : m_Scene->GetEntities())
    {
        auto* e = ep.get();
        EntitySnapshot snap;
        snap.Tag = e->GetTag();
        snap.Transform = e->GetTransform();
        snap.HasMesh = e->HasMesh();
        snap.HasMaterial = e->HasMaterial();
        snap.HasLight = e->HasLight();
        snap.HasScript = e->HasScript();

        if (snap.HasMaterial && e->GetMaterial()->Mat)
            snap.MaterialColor = e->GetMaterial()->Mat->GetColor();
        if (snap.HasLight)
            snap.Light = *e->GetLight();
        if (snap.HasScript)
            snap.Script = *e->GetScript();

        m_SceneSnapshot.push_back(snap);
    }
}

void SceneLayer::RestoreSnapshot()
{
    m_Selection.clear();
    m_Scene->Clear();

    for (auto& snap : m_SceneSnapshot)
    {
        auto* e = m_Scene->CreateEntity(snap.Tag);
        e->GetTransform() = snap.Transform;

        if (snap.HasMesh)
        {
            // Re-create mesh by name — we match by tag prefix
            std::shared_ptr<Pondo::Mesh> mesh;
            std::string& tag = snap.Tag;
            if (tag.find("Sphere") != std::string::npos) mesh = Pondo::Mesh::CreateSphere();
            else if (tag.find("Plane") != std::string::npos) mesh = Pondo::Mesh::CreatePlane(1.0f);
            else if (tag.find("Cylinder") != std::string::npos) mesh = Pondo::Mesh::CreateCylinder();
            else                                                 mesh = Pondo::Mesh::CreateCube();
            e->SetMesh(mesh);
        }
        if (snap.HasMaterial)
        {
            e->SetMaterial(std::make_shared<Pondo::Material>(m_Shader));
            e->GetMaterial()->Mat->SetColor(snap.MaterialColor);
        }
        if (snap.HasLight)
        {
            e->AddLight(snap.Light.Type);
            *e->GetLight() = snap.Light;
        }
        if (snap.HasScript)
            e->SetScript(snap.Script.ScriptPath);
    }
}

// -------------------------------------------------------
//  CSG helpers
// -------------------------------------------------------

static void RecalcNormalsSmooth(std::vector<Pondo::Vertex>& verts,
    const std::vector<unsigned int>& idx)
{
    // Zero out normals
    for (auto& v : verts)
        v.Normal = { 0.0f, 0.0f, 0.0f };

    // Accumulate face normals weighted by angle
    for (size_t i = 0; i + 2 < idx.size(); i += 3)
    {
        uint32_t i0 = idx[i], i1 = idx[i + 1], i2 = idx[i + 2];
        if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size())
            continue;

        glm::vec3 e1 = verts[i1].Position - verts[i0].Position;
        glm::vec3 e2 = verts[i2].Position - verts[i0].Position;
        glm::vec3 n = glm::cross(e1, e2); // not normalized — length = 2*area weight

        verts[i0].Normal += n;
        verts[i1].Normal += n;
        verts[i2].Normal += n;
    }

    // Normalize — guard against degenerate verts
    for (auto& v : verts)
    {
        float len = glm::length(v.Normal);
        if (len > 1e-6f)
            v.Normal /= len;
        else
            v.Normal = { 0.0f, 1.0f, 0.0f };
    }
}

void SceneLayer::Play()
{
    if (m_PlayState != PlayState::Edit) return;
    TakeSnapshot();
    m_PlayState = PlayState::Playing;
    Pondo::ScriptEngine::OnPlayStart(m_Scene.get());
}

void SceneLayer::Pause()
{
    if (m_PlayState == PlayState::Playing) m_PlayState = PlayState::Paused;
    else if (m_PlayState == PlayState::Paused) m_PlayState = PlayState::Playing;
}

void SceneLayer::Stop()
{
    if (m_PlayState == PlayState::Edit) return;
    Pondo::ScriptEngine::OnPlayStop();
    m_PlayState = PlayState::Edit;
    RestoreSnapshot();
}

// -------------------------------------------------------
//  Construction / wiring
// -------------------------------------------------------

SceneLayer::SceneLayer() : Layer("SceneLayer") {}
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

    if (!best)
    {
		if (!addToSelection) m_Selection.clear();
		return;
    }

    // If the picked entity belongs to a group, select the whole group
    if (best->InGroup())
    {
        uint32_t parentID = best->GetGroup()->ParentID;
        auto* root = m_Scene->FindEntity(parentID);
        if (root)
        {
            auto children = GetGroupChildren(parentID);
            if (!addToSelection) m_Selection.clear();
            if (std::find(m_Selection.begin(), m_Selection.end(), root) == m_Selection.end())
                m_Selection.push_back(root);
            for (auto* c : children)
                if (std::find(m_Selection.begin(), m_Selection.end(), c) == m_Selection.end())
                    m_Selection.push_back(c);
            return;
        }
    }

    if (!addToSelection) m_Selection.clear();
    m_Selection.push_back(best);
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
    Pondo::ScriptEngine::Init();
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
    if (m_PlayState == PlayState::Playing)
    {
        Pondo::ScriptEngine::OnUpdate(m_Scene.get(), ts.GetSeconds());
        return; // no camera movement / gizmo input during play
    }

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

        bool negate =
            ep->IsNegate();

        if (negate)
        {
            glm::vec4 original =
                mat->Mat->GetColor();

            mat->Mat->SetColor(
                {
                    1.0f,
                    0.55f,
                    0.55f,
                    1.0f
                });

            Pondo::Renderer::SubmitMesh(
                mc->MeshData,
                mat->Mat,
                ep->GetTransform()
                .GetTransform());

            mat->Mat->SetColor(
                original);
        }
        else
        {
            Pondo::Renderer::SubmitMesh(
                mc->MeshData,
                mat->Mat,
                ep->GetTransform()
                .GetTransform());
        }
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

void SceneLayer::NegateSelected()
{
    if (m_Selection.empty())
        return;

    for (auto* e : m_Selection)
    {
        if (!e || !e->InGroup())
            continue;

        auto* gc = e->GetGroup();
        if (!gc) continue;

        bool neg = !gc->IsNegate;
        e->SetGroup(gc->ParentID, neg);

        if (e->HasMaterial())
        {
            auto* mat = e->GetMaterial();
            if (mat && mat->Mat)
            {
                mat->Mat->SetColor(neg
                    ? glm::vec4{ 1.0f, 0.35f, 0.35f, 1.0f }
                    : glm::vec4{ 1.0f, 1.0f,  1.0f,  1.0f });
            }
        }
    }
}

Pondo::Entity* SceneLayer::CreateGroup(const std::string& name)
{
    // Snapshot IDs before CreateEntity, which may reallocate m_Entities
    std::vector<uint32_t> selIDs;
    selIDs.reserve(m_Selection.size());
    for (auto* e : m_Selection) selIDs.push_back(e->GetID());

    // Create the group root entity (no mesh)
    auto* root = m_Scene->CreateEntity(name);
    root->MakeGroupRoot(name);
    uint32_t rootID = root->GetID();

    // Parent all previously-selected entities into this group
    for (uint32_t id : selIDs)
    {
        auto* e = m_Scene->FindEntity(id);
        if (e && !e->IsGroupRoot())
            e->SetGroup(rootID, false);
    }

    m_Selection.clear();
    m_Selection.push_back(m_Scene->FindEntity(rootID));
    return m_Selection[0];
}

void SceneLayer::UngroupSelected()
{
    if (m_Selection.empty()) return;
    auto* root = m_Selection[0];
    if (!root->IsGroupRoot()) return;

    uint32_t rootID = root->GetID();

    // Remove group membership from all children
    for (auto& ep : m_Scene->GetEntities())
    {
        auto* gc = ep->GetGroup();
        if (gc && gc->ParentID == rootID)
            ep->RemoveFromGroup();
    }

    // Destroy the root entity
    m_Scene->DestroyEntity(rootID);
    m_Selection.clear();
}

static std::shared_ptr<Pondo::Mesh> SubtractMeshes(
    Pondo::Entity* positive,
    Pondo::Entity* negative)
{
    if (!positive || !negative) return nullptr;

    auto* pos = positive->GetMesh();
    auto* neg = negative->GetMesh();
    if (!pos || !neg || !pos->MeshData || !neg->MeshData) return nullptr;

    const auto& posVerts = pos->MeshData->GetVertices();
    const auto& posIndices = pos->MeshData->GetIndices();
    const auto& negVerts = neg->MeshData->GetVertices();
    const auto& negIndices = neg->MeshData->GetIndices();

    if (posVerts.empty() || posIndices.empty() ||
        negVerts.empty() || negIndices.empty() ||
        posIndices.size() % 3 != 0 || negIndices.size() % 3 != 0)
    {
        return pos->MeshData; // passthrough — nothing to cut
    }

    glm::mat4 posTf = positive->GetTransform().GetTransform();
    glm::mat4 negTf = negative->GetTransform().GetTransform();

    // World-space vertex arrays (double precision — mcut requirement)
    std::vector<double> srcCoords, cutCoords;
    srcCoords.reserve(posVerts.size() * 3);
    cutCoords.reserve(negVerts.size() * 3);

    for (auto& v : posVerts) {
        glm::vec4 wp = posTf * glm::vec4(v.Position, 1.0f);
        srcCoords.push_back(wp.x); srcCoords.push_back(wp.y); srcCoords.push_back(wp.z);
    }
    for (auto& v : negVerts) {
        glm::vec4 wp = negTf * glm::vec4(v.Position, 1.0f);
        cutCoords.push_back(wp.x); cutCoords.push_back(wp.y); cutCoords.push_back(wp.z);
    }

    std::vector<uint32_t> srcFaces, cutFaces;
    for (auto i : posIndices) srcFaces.push_back((uint32_t)i);
    for (auto i : negIndices) cutFaces.push_back((uint32_t)i);

    std::vector<uint32_t> srcFaceSizes(posIndices.size() / 3, 3);
    std::vector<uint32_t> cutFaceSizes(negIndices.size() / 3, 3);

    McContext ctx = MC_NULL_HANDLE;
    mcCreateContext(&ctx, MC_NULL_HANDLE);

    McResult r = mcDispatch(
        ctx,
        MC_DISPATCH_VERTEX_ARRAY_DOUBLE,
        srcCoords.data(), srcFaces.data(), srcFaceSizes.data(),
        (uint32_t)(srcCoords.size() / 3), (uint32_t)srcFaceSizes.size(),
        cutCoords.data(), cutFaces.data(), cutFaceSizes.data(),
        (uint32_t)(cutCoords.size() / 3), (uint32_t)cutFaceSizes.size()
    );

    if (r != MC_NO_ERROR) {
        mcReleaseContext(ctx);
        return pos->MeshData;
    }

    uint32_t numCC = 0;
    mcGetConnectedComponents(ctx, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, 0, nullptr, &numCC);

    if (numCC == 0) {
        mcReleaseContext(ctx);
        return pos->MeshData;
    }

    std::vector<McConnectedComponent> comps(numCC);
    mcGetConnectedComponents(ctx, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, numCC, comps.data(), nullptr);

    // Collect ALL above-fragments and merge them (handles disconnected results)
    std::vector<Pondo::Vertex> finalVerts;
    std::vector<unsigned int>  finalIdx;

    for (auto& cc : comps)
    {
        McFragmentLocation loc;
        mcGetConnectedComponentData(ctx, cc, MC_CONNECTED_COMPONENT_DATA_FRAGMENT_LOCATION,
            sizeof(loc), &loc, nullptr);
        if (loc != MC_FRAGMENT_LOCATION_ABOVE)
            continue;

        // Extract vertices
        uint64_t vertBytes = 0;
        mcGetConnectedComponentData(ctx, cc, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE,
            0, nullptr, &vertBytes);
        std::vector<double> outVerts(vertBytes / sizeof(double));
        mcGetConnectedComponentData(ctx, cc, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE,
            vertBytes, outVerts.data(), nullptr);

        // Extract triangulated indices
        uint64_t idxBytes = 0;
        mcGetConnectedComponentData(ctx, cc, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION,
            0, nullptr, &idxBytes);
        std::vector<uint32_t> outIdx(idxBytes / sizeof(uint32_t));
        mcGetConnectedComponentData(ctx, cc, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION,
            idxBytes, outIdx.data(), nullptr);

        if (outVerts.empty() || outIdx.empty()) continue;

        uint32_t base = (uint32_t)finalVerts.size();

        for (size_t i = 0; i + 2 < outVerts.size(); i += 3) {
            Pondo::Vertex v;
            v.Position = { (float)outVerts[i], (float)outVerts[i + 1], (float)outVerts[i + 2] };
            v.Normal = { 0.0f, 1.0f, 0.0f }; // recalculated below
            v.TexCoord = { 0.0f, 0.0f };
            finalVerts.push_back(v);
        }
        for (auto i : outIdx)
            finalIdx.push_back((unsigned int)(i + base));
    }

    mcReleaseContext(ctx);

    if (finalVerts.empty() || finalIdx.empty())
        return pos->MeshData; // mcut gave nothing usable

    // Smooth normals across the whole merged result
    RecalcNormalsSmooth(finalVerts, finalIdx);

    return std::make_shared<Pondo::Mesh>(finalVerts, finalIdx);
}

static std::shared_ptr<Pondo::Mesh> CombineMeshes(
    Pondo::Entity* a,
    Pondo::Entity* b)
{
    glm::mat4 tfA = a->GetTransform().GetTransform();
    const auto& rawVertsA = a->GetMesh()->MeshData->GetVertices();
    std::vector<Pondo::Vertex> resultVerts;
    resultVerts.reserve(rawVertsA.size());
    for (auto v : rawVertsA) {
        glm::vec4 p = tfA * glm::vec4(v.Position, 1.0f);
        v.Position = { p.x, p.y, p.z };
        resultVerts.push_back(v);
    }

    auto resultIdx =
        a->GetMesh()->MeshData->GetIndices();

    auto& vertsB =
        b->GetMesh()->MeshData->GetVertices();

    auto& idxB =
        b->GetMesh()->MeshData->GetIndices();

    uint32_t base =
        (uint32_t)resultVerts.size();

    glm::mat4 tf =
        b->GetTransform().GetTransform();

    for (auto v : vertsB)
    {
        glm::vec4 p =
            tf *
            glm::vec4(
                v.Position,
                1.0f);

        v.Position =
        {
            p.x,
            p.y,
            p.z
        };

        resultVerts.push_back(v);
    }

    for (auto i : idxB)
    {
        resultIdx.push_back(
            i + base);
    }

    // Recalculate normals so seams between combined parts look correct
    RecalcNormalsSmooth(resultVerts, resultIdx);

    return std::make_shared<Pondo::Mesh>(resultVerts, resultIdx);
}

SceneLayer::UnionResult SceneLayer::UnionGroup()
{
    if (m_Selection.empty())
        return UnionResult::NoChildren;

    auto* root = m_Selection[0];

    if (!root->IsGroupRoot())
        return UnionResult::NoChildren;

    uint32_t rootID = root->GetID();

    auto children = GetGroupChildren(rootID);

    std::vector<Pondo::Entity*> positives;
    std::vector<Pondo::Entity*> negatives;

    for (auto* e : children)
    {
        auto* mesh = e->GetMesh();
        if (!mesh || !mesh->MeshData)
            continue;

        if (e->IsNegate())
            negatives.push_back(e);
        else
            positives.push_back(e);
    }

    if (positives.empty())
        return UnionResult::NoPositiveParts;

    //
    // STEP 1 — combine positives into a temporary scratch entity
    //

    auto* tempRoot = m_Scene->CreateEntity("__union");
    uint32_t tempRootID = tempRoot->GetID();

    // NOTE: CreateEntity may reallocate m_Entities.
    // Re-fetch root and all child pointers via ID now.
    root = m_Scene->FindEntity(rootID);

    for (size_t i = 0; i < positives.size(); i++)
        positives[i] = m_Scene->FindEntity(positives[i]->GetID());
    for (size_t i = 0; i < negatives.size(); i++)
        negatives[i] = m_Scene->FindEntity(negatives[i]->GetID());

    // Re-fetch tempRoot too (FindEntity is safe)
    tempRoot = m_Scene->FindEntity(tempRootID);

    tempRoot->SetMesh(positives[0]->GetMesh()->MeshData);
    tempRoot->GetTransform() = positives[0]->GetTransform();

    for (size_t i = 1; i < positives.size(); i++)
    {
        auto combined = CombineMeshes(tempRoot, positives[i]);
        if (combined)
            tempRoot->SetMesh(combined);
        tempRoot->GetTransform() = {};
    }

    //
    // STEP 2 — subtract negatives
    //

    for (auto* neg : negatives)
    {
        auto cut = SubtractMeshes(tempRoot, neg);
        if (!cut || cut->GetVertices().empty() || cut->GetIndices().empty())
            continue;
        tempRoot->SetMesh(cut);
        tempRoot->GetTransform() = {};
    }

    //
    // STEP 3 — read all results out before any destruction
    //

    std::shared_ptr<Pondo::Mesh>     finalMesh;
    std::shared_ptr<Pondo::Material> finalMat;

    if (tempRoot->GetMesh())
        finalMesh = tempRoot->GetMesh()->MeshData;

    if (positives[0]->GetMaterial() && positives[0]->GetMaterial()->Mat)
        finalMat = positives[0]->GetMaterial()->Mat;

    // Collect child IDs to destroy (everything except root)
    std::vector<uint32_t> toDestroy;
    toDestroy.push_back(tempRootID);
    for (auto* child : children)
        toDestroy.push_back(child->GetID()); // root's own ID included — filtered below

    //
    // STEP 4 — destroy temp + children (not root), then apply result via ID lookup
    //

    for (uint32_t id : toDestroy)
    {
        if (id != rootID)
            m_Scene->DestroyEntity(id);
    }

    // Re-fetch root — erasures above may have shifted the vector
    root = m_Scene->FindEntity(rootID);
    if (!root) return UnionResult::MeshOperationFailed;

    if (finalMesh)
    {
        root->SetMesh(finalMesh);
        root->GetTransform() = {};
    }

    if (finalMat)
    {
        root->SetMaterial(std::make_shared<Pondo::Material>(m_Shader));
        root->GetMaterial()->Mat = finalMat;
    }

    if (auto* gr = root->GetGroupRoot())
        gr->Unioned = true;

    m_Selection.clear();
    m_Selection.push_back(root);
    return UnionResult::OK;
}

void SceneLayer::SeparateSelected()
{
    if (m_Selection.empty())
        return;

    auto* root = m_Selection[0];
    if (!root || !root->IsGroupRoot())
        return;

    // Separate can't reconstruct destroyed children — instead just ungroup
    // the root so it becomes a standalone mesh entity again.
    uint32_t rootID = root->GetID();

    m_Selection.clear();
    auto* e = m_Scene->FindEntity(rootID);
    if (e) m_Selection.push_back(e);
}

std::vector<Pondo::Entity*>
SceneLayer::GetGroupChildren(
    uint32_t groupRootID) const
{
    std::vector<Pondo::Entity*> children;

    for (auto& ep : m_Scene->GetEntities())
    {
        auto* gc =
            ep->GetGroup();

        if (
            gc &&
            gc->ParentID ==
            groupRootID)
        {
            children.push_back(
                ep.get());
        }
    }

    return children;
}