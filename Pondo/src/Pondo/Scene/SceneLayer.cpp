#include "SceneLayer.h"
#include "Pondo/Renderer/RayUtils.h"

#include <Pondo.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Pondo/Scene/SceneSerializer.h"

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
    m_SelectedEntity = nullptr;
    m_Scene->Clear();
    Pondo::SceneSerializer::Load(m_Scene.get(), path, m_Shader);

    auto& entities = m_Scene->GetEntities();
    if (!entities.empty())
        m_SelectedEntity = entities[0].get();
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
                std::max(
                    lc->Intensity,
                    8.0f);

            out.Constant =
                1.0f;

            out.Linear =
                0.01f;

            out.Quadratic =
                0.0005f;

            out.Range = lc->Range;
        }

        if (lc->Type == Pondo::LightType::Spot)
        {
            if (m_Lights.SpotLightCount >=
                Pondo::MAX_SPOT_LIGHTS)
                continue;

            auto& out =
                m_Lights.SpotLights[
                    m_Lights.SpotLightCount++
                ];

            out.Position =
                tf.Position;

            out.Direction =
                lc->Direction;

            if (glm::length(lc->Color) < 0.01f)
                out.Color = { 1.0f, 1.0f, 1.0f };
            else
                out.Color = lc->Color;

            out.Intensity =
                std::max(
                    lc->Intensity,
                    8.0f);

            out.Constant =
                lc->Constant;

            out.Linear =
                lc->Linear;

            out.Quadratic =
                lc->Quadratic;
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
        m_Scene->CreateEntity(name);

    e->GetTransform().Position =
        pos;

    e->AddLight(type);

    return e;
}

Pondo::Entity* SceneLayer::DuplicateEntity(Pondo::Entity* src)
{
    if (!src) return nullptr;

    glm::vec4 color = { 1, 1, 1, 1 };
    if (src->GetMaterial() && src->GetMaterial()->Mat)
        color = src->GetMaterial()->Mat->GetColor();

    auto* copy = SpawnEntity(
        src->GetTag() + "_Copy",
        src->GetMesh()->MeshData,
        color,
        src->GetTransform().Position);

    copy->GetTransform() = src->GetTransform();
    return copy;
}

void SceneLayer::DeleteSelected()
{
    if (!m_SelectedEntity) return;
    uint32_t id = m_SelectedEntity->GetID();
    m_SelectedEntity = nullptr;
    m_Scene->DestroyEntity(id);
}

// -------------------------------------------------------
//  Picking
// -------------------------------------------------------

void SceneLayer::TryPickEntity(glm::vec2 px)
{
    if (m_VpSize.x <= 0 || m_VpSize.y <= 0) return;
    Ray ray = ScreenToRay(px, m_VpPos, m_VpSize,
        glm::inverse(m_Camera->GetViewProjection()));

    Pondo::Entity* best = nullptr;
    float bestT = 1e30f;
    for (auto& ep : m_Scene->GetEntities()) {
        if (!ep->GetMesh()) continue;
        glm::vec3 c = ep->GetTransform().Position;
        glm::vec3 h = glm::abs(ep->GetTransform().Scale) * 0.5f;
        float t = RayAABB(ray, c - h, c + h);
        if (t > 0.0f && t < bestT) { bestT = t; best = ep.get(); }
    }
    m_SelectedEntity = best;
}

// -------------------------------------------------------
//  Gizmo hit-testing
// -------------------------------------------------------

int SceneLayer::GizmoAxisHit(glm::vec2 px)
{
    if (!m_SelectedEntity) return -1;

    glm::vec3 origin = m_SelectedEntity->GetTransform().Position;
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
        return bestDist < 40.0f ? bestAxis : -1;
    }

    // --- MOVE + SCALE MODE ---
    glm::vec3 tips[3] = {
        origin + glm::vec3(gizmoScale, 0, 0),
        origin + glm::vec3(0, gizmoScale, 0),
        origin + glm::vec3(0, 0, gizmoScale)
    };

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
    return bestDist < 35.0f ? bestAxis : -1;
}

// -------------------------------------------------------
//  Gizmo drag
// -------------------------------------------------------

void SceneLayer::BeginGizmoDrag(int axis, glm::vec2 px)
{
    if (!m_SelectedEntity) return;
    m_DragAxis = axis;
    m_DragStartPx = px;
    auto& tf = m_SelectedEntity->GetTransform();
    m_DragStartPos = tf.Position;
    m_DragStartScale = tf.Scale;
    m_DragStartRot = tf.Rotation;
}

void SceneLayer::UpdateGizmoDrag(glm::vec2 px, bool enableSnap,
    float moveIncrement, float rotateIncrement, float scaleIncrement)
{
    if (m_DragAxis < 0 || !m_SelectedEntity) return;

    auto& transform = m_SelectedEntity->GetTransform();
    static const glm::vec3 axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    glm::vec3 axis = axes[m_DragAxis];

    glm::vec2 p0 = WorldToScreen(m_DragStartPos, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
    glm::vec2 p1 = WorldToScreen(m_DragStartPos + axis, m_Camera->GetViewProjection(), m_VpPos, m_VpSize);
    glm::vec2 dir = p1 - p0;
    float     len = glm::length(dir);
    if (len < 1.0f) return;
    dir /= len;

    float dragAmount = glm::dot(px - m_DragStartPx, dir) * 0.01f;

    switch (m_GizmoMode)
    {
    case 0: // Move
    {
        if (enableSnap && moveIncrement > 0.0f)
            dragAmount = round(dragAmount / moveIncrement) * moveIncrement;
        transform.Position = m_DragStartPos + axis * dragAmount;
        break;
    }
    case 1: // Rotate
    {
        float rotation = dragAmount * 100.0f;
        if (enableSnap && rotateIncrement > 0.0f)
            rotation = round(rotation / rotateIncrement) * rotateIncrement;
        transform.Rotation[m_DragAxis] = m_DragStartRot[m_DragAxis] + rotation;
        break;
    }
    case 2: // Scale
    {
        if (enableSnap && scaleIncrement > 0.0f)
            dragAmount = round(dragAmount / scaleIncrement) * scaleIncrement;
        transform.Scale[m_DragAxis] =
            std::max(0.001f, m_DragStartScale[m_DragAxis] + dragAmount);
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

    if (m_SelectedEntity) {
        auto& tf = m_SelectedEntity->GetTransform();
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        m_FlatShader->Bind();
        m_FlatShader->SetMat4("u_ViewProjection", m_Camera->GetViewProjection());
        glm::mat4 transform = tf.GetTransform() * glm::scale(glm::mat4(1), glm::vec3(1.05f));
        m_FlatShader->SetMat4("u_Transform", transform);
        m_FlatShader->SetFloat4("u_Color", { 1, 0.85f, 0.1f, 1 });
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

void SceneLayer::OnEvent(Pondo::Event& e)
{
    Pondo::EventDispatcher d(e);
    d.Dispatch<Pondo::WindowResizeEvent>([this](Pondo::WindowResizeEvent& ev) {
        if (ev.GetWidth() > 0 && ev.GetHeight() > 0)
            m_Camera->SetAspectRatio((float)ev.GetWidth() / ev.GetHeight());
        return false;
        });
}

// -------------------------------------------------------
//  Private gizmo drawing
// -------------------------------------------------------

void SceneLayer::DrawGizmo()
{
    if (!m_SelectedEntity) return;

    if (Pondo::Input::IsKeyPressed(GLFW_KEY_1)) m_GizmoMode = 0;
    if (Pondo::Input::IsKeyPressed(GLFW_KEY_2)) m_GizmoMode = 1;
    if (Pondo::Input::IsKeyPressed(GLFW_KEY_3)) m_GizmoMode = 2;

    glm::vec3 origin = m_SelectedEntity->GetTransform().Position;
    float     scale = glm::length(m_Camera->GetPosition() - origin) * 0.15f;

    glDisable(GL_DEPTH_TEST);

    struct Axis { glm::vec3 dir; glm::vec4 color; glm::vec4 active; };
    Axis axes[3] = {
        { {1,0,0}, {1,0.2f,0.2f,1}, {1,0.6f,0.6f,1} },
        { {0,1,0}, {0.2f,1,0.2f,1}, {0.6f,1,0.6f,1} },
        { {0,0,1}, {0.2f,0.4f,1,1}, {0.6f,0.8f,1,1} }
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

void SceneLayer::DrawRotationCircle(const glm::vec3& origin, const glm::vec3& axis,
    float scale, const glm::vec4& color)
{
    constexpr int SEGMENTS = 48;
    std::vector<float> verts;
    float     radius = scale * 0.9f;
    glm::vec3 right, up;

    if (axis.x != 0) { right = { 0,1,0 }; up = { 0,0,1 }; }
    else if (axis.y != 0) { right = { 1,0,0 }; up = { 0,0,1 }; }
    else { right = { 1,0,0 }; up = { 0,1,0 }; }

    for (int i = 0; i <= SEGMENTS; i++) {
        float     a = glm::two_pi<float>() * ((float)i / SEGMENTS);
        glm::vec3 p = origin + right * cos(a) * radius + up * sin(a) * radius;
        verts.push_back(p.x); verts.push_back(p.y); verts.push_back(p.z);
    }

    auto vb = std::make_shared<Pondo::VertexBuffer>(verts.data(),
        (uint32_t)(verts.size() * sizeof(float)));
    auto va = std::make_shared<Pondo::VertexArray>();
    va->AddVertexBuffer(vb);

    m_FlatShader->Bind();
    m_FlatShader->SetMat4("u_Transform", glm::mat4(1));
    m_FlatShader->SetFloat4("u_Color", color);
    va->Bind();
    glLineWidth(4);
    glDrawArrays(GL_LINE_STRIP, 0, SEGMENTS + 1);
    va->Unbind();
    glLineWidth(1);
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