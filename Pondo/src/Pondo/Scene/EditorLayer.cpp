#include "EditorLayer.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>

#include "Pondo/ImGui/ImGuiKeyMap.h"
#include "FileDialogs.h"
#include "Pondo/Scene/SceneSerializer.h"
#include "Pondo/Events/ApplicationEvents.h"
#include "Pondo/Events/KeyEvents.h"
#include "Pondo/Events/MouseEvents.h"

// -------------------------------------------------------
//  Construction
// -------------------------------------------------------

EditorLayer::EditorLayer(SceneLayer* sl)
    : Layer("EditorLayer"), m_SceneLayer(sl)
{
}

// -------------------------------------------------------
//  OnAttach — init ImGui (manual IO, OpenGL renderer only)
// -------------------------------------------------------

void EditorLayer::OnAttach()
{
    m_Window = static_cast<GLFWwindow*>(
        Pondo::Application::Get().GetWindow().GetNativeWindow());

    int w = Pondo::Application::Get().GetWindow().GetWidth();
    int h = Pondo::Application::Get().GetWindow().GetHeight();

    Pondo::FramebufferSpec spec;
    spec.Width  = (unsigned)w;
    spec.Height = (unsigned)h;
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
    s.WindowRounding = 4.0f;
    s.FrameRounding  = 3.0f;
    s.Colors[ImGuiCol_WindowBg]       = { 0.13f, 0.13f, 0.14f, 1 };
    s.Colors[ImGuiCol_TitleBg]        = { 0.10f, 0.10f, 0.11f, 1 };
    s.Colors[ImGuiCol_TitleBgActive]  = { 0.16f, 0.29f, 0.48f, 1 };
    s.Colors[ImGuiCol_Header]         = { 0.20f, 0.35f, 0.55f, 0.6f };
    s.Colors[ImGuiCol_HeaderHovered]  = { 0.26f, 0.47f, 0.70f, 0.8f };
    s.Colors[ImGuiCol_FrameBg]        = { 0.18f, 0.18f, 0.20f, 1 };
    s.Colors[ImGuiCol_Button]         = { 0.20f, 0.35f, 0.55f, 1 };
    s.Colors[ImGuiCol_ButtonHovered]  = { 0.26f, 0.47f, 0.70f, 1 };

    ImGui_ImplOpenGL3_Init("#version 450");
}

void EditorLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

// -------------------------------------------------------
//  OnEvent — forward Pondo events into ImGuiIO
// -------------------------------------------------------

void EditorLayer::OnEvent(Pondo::Event& e)
{
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

    d.Dispatch<Pondo::KeyPressedEvent>([this, &io](Pondo::KeyPressedEvent& ev) {
        ImGuiKey key = GlfwKeyToImGui(ev.GetKeyCode());
        if (key != ImGuiKey_None) io.AddKeyEvent(key, true);
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

    d.Dispatch<Pondo::KeyTypedEvent>([&io](Pondo::KeyTypedEvent& ev) {
        io.AddInputCharacter((unsigned int)ev.GetKeyCode());
        return false;
        });
}

// -------------------------------------------------------
//  OnRender — full ImGui frame
// -------------------------------------------------------

void EditorLayer::OnRender()
{
    int w = Pondo::Application::Get().GetWindow().GetWidth();
    int h = Pondo::Application::Get().GetWindow().GetHeight();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = { (float)w, (float)h };
    io.DisplayFramebufferScale = { 1, 1 };

    float now = (float)glfwGetTime();
    io.DeltaTime = (m_LastTime > 0.0f) ? (now - m_LastTime) : (1.0f / 60.0f);
    if (io.DeltaTime <= 0.0f) io.DeltaTime = 1.0f / 60.0f;
    m_LastTime = now;

    { auto [mx, my] = Pondo::Input::GetMousePosition(); io.AddMousePosEvent(mx, my); }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Ctrl+Z / Ctrl+Y
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) Undo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) Redo();

    // Ctrl+S quick-save
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
        if (m_CurrentScenePath.empty()) m_CurrentScenePath = SaveFileDialog();
        if (!m_CurrentScenePath.empty())
            Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), m_CurrentScenePath);
    }

    // Global / Local space hint toast
    if (m_SceneLayer->GetGizmoHintTimer() > 0)
    {
        m_SceneLayer->TickGizmoHint(io.DeltaTime);

        const float PAD = 12.0f;
        ImVec2 displaySize = io.DisplaySize;
        ImGui::SetNextWindowBgAlpha(0.82f);
        // Anchor to bottom-right corner so it never blocks the scene panel or properties
        ImGui::SetNextWindowPos(
            { displaySize.x - PAD, displaySize.y - PAD },
            ImGuiCond_Always,
            { 1.0f, 1.0f });

        ImGui::Begin(
            "##TransformSpace",
            nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove       |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings  |
            ImGuiWindowFlags_NoFocusOnAppearing);

        ImGui::Text("Transform Space");
        ImGui::Separator();

        if (m_SceneLayer->GetGizmoSpace() == 0)
            ImGui::TextColored({ 1, 0.8f, 0.2f, 1 }, "GLOBAL");
        else
            ImGui::TextColored({ 0.2f, 0.9f, 1, 1 }, "LOCAL");

        ImGui::End();
    }

    // ── Menu bar ─────────────────────────────────────────────────────
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize({ (float)w, 0 });
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("##Menu", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(2);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene")) {
                std::string path = SaveFileDialog();
                if (!path.empty()) Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), path);
            }
            if (ImGui::MenuItem("Load Scene")) {
                std::string path = OpenFileDialog();
                if (!path.empty()) m_SceneLayer->LoadScene(path);
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (m_CurrentScenePath.empty()) m_CurrentScenePath = SaveFileDialog();
                if (!m_CurrentScenePath.empty())
                    Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), m_CurrentScenePath);
            }
            if (ImGui::MenuItem("Save As")) {
                std::string path = SaveFileDialog();
                if (!path.empty()) {
                    m_CurrentScenePath = path;
                    Pondo::SceneSerializer::Save(m_SceneLayer->GetScene(), path);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) Pondo::Application::Get().Close();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Scene",      nullptr, &m_ShowScene);
            ImGui::MenuItem("Properties", nullptr, &m_ShowProps);
            ImGui::MenuItem("Stats",      nullptr, &m_ShowStats);
            ImGui::MenuItem("Lighting",   nullptr, &m_ShowLighting);  // NEW
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Cube"))     CreateEntity(0);
            if (ImGui::MenuItem("Sphere"))   CreateEntity(1);
            if (ImGui::MenuItem("Plane"))    CreateEntity(2);
            if (ImGui::MenuItem("Cylinder")) CreateEntity(3);
            ImGui::Separator();                                                                          // NEW
            if (ImGui::MenuItem("Point Light"))       m_SceneLayer->SpawnLight("PointLight",  Pondo::LightType::Point);       // NEW
            if (ImGui::MenuItem("Spot Light"))        m_SceneLayer->SpawnLight("SpotLight",   Pondo::LightType::Spot);        // NEW
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    float menuH = ImGui::GetWindowHeight();
    ImGui::End();

    const float sideW    = 260.0f;
    const float topY     = menuH;
    const float contentH = (float)h - topY;
    float leftW  = m_ShowScene ? sideW : 0.0f;
    float rightW = (m_ShowProps || m_ShowStats) ? sideW : 0.0f;

    // ── Scene panel ──────────────────────────────────────────────────
    if (m_ShowScene)
    {
        // Give the scene panel the top 55% of the left column so the
        // lighting panel can sit below it.
        float sceneH = m_ShowLighting ? contentH * 0.55f : contentH;

        ImGui::SetNextWindowPos({ 0, topY });
        ImGui::SetNextWindowSize({ sideW, sceneH });
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
        if (ImGui::Button("+ Cube"))     CreateEntity(0); ImGui::SameLine();
        if (ImGui::Button("+ Sphere"))   CreateEntity(1); ImGui::SameLine();
        if (ImGui::Button("+ Plane"))    CreateEntity(2);
        if (ImGui::Button("+ Cylinder")) CreateEntity(3);

        if (m_SceneLayer->GetSelectedEntity()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button,        { 0.65f, 0.15f, 0.15f, 1 });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.80f, 0.20f, 0.20f, 1 });
            if (ImGui::Button("Delete Selected")) m_SceneLayer->DeleteSelected();
            ImGui::PopStyleColor(2);
        }
        ImGui::End();
    }

    // ── Lighting panel (like Roblox Studio's Lighting service) ───────
    if (m_ShowLighting)
    {
        float litY = m_ShowScene ? topY + contentH * 0.55f : topY;
        float litH = m_ShowScene ? contentH * 0.45f        : contentH;

        ImGui::SetNextWindowPos({ 0, litY });
        ImGui::SetNextWindowSize({ sideW, litH });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Lighting##panel", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        auto& L = m_SceneLayer->GetLightEnvironment();

        if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float amb[3] = { L.AmbientColor.r, L.AmbientColor.g, L.AmbientColor.b };
            if (ImGui::ColorEdit3("Ambient Color", amb))
                L.AmbientColor = { amb[0], amb[1], amb[2] };
            ImGui::SliderFloat("Ambient Intensity", &L.AmbientIntensity, 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Sun Active", &L.Sun.Active);
            float sunCol[3] = { L.Sun.Color.r, L.Sun.Color.g, L.Sun.Color.b };
            if (ImGui::ColorEdit3("Sun Color", sunCol))
                L.Sun.Color = { sunCol[0], sunCol[1], sunCol[2] };
            ImGui::SliderFloat("Sun Intensity",  &L.Sun.Intensity, 0.0f, 5.0f);
            ImGui::SliderFloat3("Sun Direction", &L.Sun.Direction.x, -1.0f, 1.0f);
            if (ImGui::Button("Normalize Direction"))
                L.Sun.Direction = glm::normalize(L.Sun.Direction);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Point / Spot lights: Create menu");
        ImGui::TextDisabled("Select a light to edit in Properties.");

        ImGui::End();
    }

    // ── Properties panel ─────────────────────────────────────────────
    if (m_ShowProps || m_ShowStats)
    {
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
            ImGui::Text("Gizmo:"); ImGui::Text("1 Move"); ImGui::Text("2 Rotate"); ImGui::Text("3 Scale");
        }

        if (m_ShowProps) {
            if (Pondo::Entity* sel = m_SceneLayer->GetSelectedEntity()) {
                char buf[128]; strncpy_s(buf, sel->GetTag().c_str(), sizeof(buf));
                if (ImGui::InputText("Name", buf, sizeof(buf))) sel->SetTag(buf);
                ImGui::Separator();
                ImGui::Text("Transform Controls");
                ImGui::Separator();

                ImGui::Checkbox("Enable Increment Snap", &m_EnableSnapping);
                ImGui::InputFloat("Move Step",   &m_MoveIncrement,   0, 0, "%.3f");
                ImGui::InputFloat("Rotate Step", &m_RotateIncrement, 0, 0, "%.2f");
                ImGui::InputFloat("Scale Step",  &m_ScaleIncrement,  0, 0, "%.3f");
                m_MoveIncrement   = std::max(0.001f, m_MoveIncrement);
                m_RotateIncrement = std::max(0.01f,  m_RotateIncrement);
                m_ScaleIncrement  = std::max(0.001f, m_ScaleIncrement);

                ImGui::Separator();
                auto& tf = sel->GetTransform();

                if (ImGui::IsWindowFocused() && !m_WasEditingTransform) {
                    const auto& sel = m_SceneLayer->GetSelection();
                    m_TransformsBefore.clear();
                    for (auto* e : sel) m_TransformsBefore.push_back(e->GetTransform());
                }

                bool changed = false;
                changed |= ImGui::DragFloat3("Position", &tf.Position.x, 0.1f,  0.0f, 0.0f, "%.3f");
                changed |= ImGui::DragFloat3("Rotation", &tf.Rotation.x, 1.0f,  0.0f, 0.0f, "%.2f");
                changed |= ImGui::DragFloat3("Scale",    &tf.Scale.x,    0.01f, 0.0f, 0.0f, "%.3f");
                tf.Scale.x = std::max(0.001f, tf.Scale.x);
                tf.Scale.y = std::max(0.001f, tf.Scale.y);
                tf.Scale.z = std::max(0.001f, tf.Scale.z);

                if (changed) m_WasEditingTransform = true;

                if (m_WasEditingTransform && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    const auto& sel = m_SceneLayer->GetSelection();
                    if (!m_TransformsBefore.empty()) {
                        if (sel.size() == 1) {
                            if (memcmp(&m_TransformsBefore[0], &tf, sizeof(tf)) != 0)
                                Execute(std::make_unique<TransformCommand>(sel[0], m_TransformsBefore[0], tf));
                        } else {
                            std::vector<EntityTransformSnapshot> snaps;
                            for (size_t i = 0; i < sel.size() && i < m_TransformsBefore.size(); i++)
                                snaps.push_back({ sel[i], m_TransformsBefore[i], sel[i]->GetTransform() });
                            Execute(std::make_unique<MultiTransformCommand>(std::move(snaps)));
                        }
                    }
                    m_WasEditingTransform = false;
                }

                // ---- Material ----
                if (auto* mc = sel->GetMaterial(); mc && mc->Mat) {
                    ImGui::Separator();
                    ImGui::Text("Material");
                    glm::vec4 col = mc->Mat->GetColor();
                    float ca[4] = { col.r, col.g, col.b, col.a };
                    if (ImGui::ColorEdit4("Color", ca))
                        mc->Mat->SetColor({ ca[0], ca[1], ca[2], ca[3] });
                }

                // ---- Light ----
                if (auto* lc = sel->GetLight(); lc)
                {
                    ImGui::Separator();
                    ImGui::Text("Light");

                    ImGui::Checkbox("Enabled", &lc->Enabled);

                    const char* lightTypes[] = { "Point", "Spot" };
                    int typeIndex = (int)lc->Type;
                    if (ImGui::Combo("Type", &typeIndex, lightTypes, 2))
                        lc->Type = (Pondo::LightType)typeIndex;

                    float col[3] = { lc->Color.r, lc->Color.g, lc->Color.b };
                    if (ImGui::ColorEdit3("Light Color", col))
                        lc->Color = { col[0], col[1], col[2] };

                    ImGui::InputFloat("Intensity", &lc->Intensity, 0.0f, 10.0f);

                    if (lc->Type == Pondo::LightType::Directional)
                    {
                        ImGui::SliderFloat3("Direction", &lc->Direction.x, -1.0f, 1.0f);
                        if (ImGui::Button("Normalize##dir"))
                            lc->Direction = glm::normalize(lc->Direction);
                    }
                    else
                    {
                        ImGui::InputFloat("Range",     &lc->Range,     0.1f,  50.0f);
                    }

                    if (lc->Type == Pondo::LightType::Spot)
                    {
                        ImGui::SliderFloat3("Direction##spot", &lc->Direction.x, -1.0f, 1.0f);
                        if (ImGui::Button("Normalize##spot"))
                            lc->Direction = glm::normalize(lc->Direction);
                        ImGui::SliderFloat("Inner Angle", &lc->InnerCutoffDeg, 1.0f, 45.0f);
                        ImGui::SliderFloat("Outer Angle", &lc->OuterCutoffDeg, 1.0f, 60.0f);
                        if (lc->OuterCutoffDeg < lc->InnerCutoffDeg)
                            lc->OuterCutoffDeg = lc->InnerCutoffDeg + 1.0f;
                    }

                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button,        { 0.5f, 0.15f, 0.15f, 1 });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.7f, 0.20f, 0.20f, 1 });
                    if (ImGui::Button("Remove Light")) sel->RemoveLight();
                    ImGui::PopStyleColor(2);
                }
                else
                {
                    // Let the user add a light to any existing entity
                    ImGui::Separator();
                    if (ImGui::Button("+ Point Light"))
                        sel->AddLight(Pondo::LightType::Point);
                    ImGui::SameLine();
                    if (ImGui::Button("+ Spot Light"))
                        sel->AddLight(Pondo::LightType::Spot);
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

    // ── Viewport ─────────────────────────────────────────────────────
    float vpX = leftW;
    float vpW = (float)w - leftW - rightW;
    ImGui::SetNextWindowPos({ vpX, topY });
    ImGui::SetNextWindowSize({ vpW, contentH });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("Viewport##panel", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse   | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar(2);

    ImVec2 contentPos = ImGui::GetCursorScreenPos();
    ImVec2 panelSize  = ImGui::GetContentRegionAvail();

    if (panelSize.x > 0 && panelSize.y > 0)
    {
        unsigned pw = (unsigned)panelSize.x, ph = (unsigned)panelSize.y;
        if (pw != m_Framebuffer->GetSpec().Width || ph != m_Framebuffer->GetSpec().Height) {
            m_Framebuffer->Resize(pw, ph);
            Pondo::WindowResizeEvent ev(pw, ph);
            m_SceneLayer->OnEvent(ev);
        }

        m_SceneLayer->SetViewportRect(contentPos.x, contentPos.y, panelSize.x, panelSize.y);

        ImGui::Image(
            (ImTextureID)(uintptr_t)m_Framebuffer->GetColorAttachmentID(),
            panelSize, { 0, 1 }, { 1, 0 });

        glm::vec2 mouse = { io.MousePos.x, io.MousePos.y };
        bool overViewport = mouse.x >= contentPos.x && mouse.x < contentPos.x + panelSize.x &&
                            mouse.y >= contentPos.y && mouse.y < contentPos.y + panelSize.y;
        bool lmbDown    = io.MouseDown[0];
        bool lmbClicked = io.MouseClicked[0];
        bool rmbDown    = io.MouseDown[1];
        bool dragging   = m_SceneLayer->IsDraggingGizmo();

        m_SceneLayer->SetViewportFocused(overViewport && !dragging);

        // Ctrl+B → Duplicate
        if (overViewport && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B, false)) {
            Pondo::Entity* copy = m_SceneLayer->DuplicateEntity(m_SceneLayer->GetSelectedEntity());
            if (copy) m_SceneLayer->SetSelectedEntity(copy);
        }

        // Update or end active drag
        if (dragging) {
            if (lmbDown) {
                m_SceneLayer->UpdateGizmoDrag(mouse, m_EnableSnapping,
                    m_MoveIncrement, m_RotateIncrement, m_ScaleIncrement);
            }
            else {
                if (m_SceneLayer->EndGizmoDrag()) {
                    const auto& sel = m_SceneLayer->GetSelection();
                    if (sel.size() == 1) {
                        Execute(std::make_unique<TransformCommand>(
                            sel[0], m_TransformsBefore[0], sel[0]->GetTransform()));
                    }
                    else if (sel.size() > 1) {
                        std::vector<EntityTransformSnapshot> snaps;
                        snaps.reserve(sel.size());
                        for (size_t i = 0; i < sel.size(); i++)
                            snaps.push_back({ sel[i], m_TransformsBefore[i], sel[i]->GetTransform() });
                        Execute(std::make_unique<MultiTransformCommand>(std::move(snaps)));
                    }
                }
            }
        }
        // Start drag or pick
        else if (overViewport && lmbClicked && !rmbDown) {
            int axis = m_SceneLayer->GizmoAxisHit(mouse);
            if (axis >= 0) {
                const auto& sel = m_SceneLayer->GetSelection();
                m_TransformsBefore.clear();
                for (auto* e : sel) m_TransformsBefore.push_back(e->GetTransform());
                m_SceneLayer->BeginGizmoDrag(axis, mouse);
            }
            else {
                bool shiftHeld = Pondo::Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                                 Pondo::Input::IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);
                m_SceneLayer->TryPickEntity(mouse, shiftHeld);
            }
        }
    }

    // Status toast
    if (m_StatusTimer > 0.0f) {
        m_StatusTimer -= io.DeltaTime;
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::SetNextWindowPos({ (float)w * 0.5f - 60, 40 });
        ImGui::Begin("##status", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("%s", m_StatusText.c_str());
        ImGui::End();
    }

    ImGui::End(); // Viewport

    // ── Flush ─────────────────────────────────────────────────────────
    glDisable(GL_DEPTH_TEST);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glEnable(GL_DEPTH_TEST);
}

// -------------------------------------------------------
//  Command history
// -------------------------------------------------------

void EditorLayer::Execute(std::unique_ptr<ICommand> cmd)
{
    while ((int)m_History.size() > m_HistoryIndex + 1)
        m_History.pop_back();
    cmd->Redo();
    m_History.push_back(std::move(cmd));
    m_HistoryIndex++;
}

void EditorLayer::Undo()
{
    if (m_HistoryIndex < 0) return;
    m_History[m_HistoryIndex]->Undo();
    m_HistoryIndex--;
    m_StatusText  = "Undo";
    m_StatusTimer = 1.0f;
}

void EditorLayer::Redo()
{
    if (m_HistoryIndex + 1 >= (int)m_History.size()) return;
    m_HistoryIndex++;
    m_History[m_HistoryIndex]->Redo();
    m_StatusText  = "Redo";
    m_StatusTimer = 1.0f;
}

// -------------------------------------------------------
//  Entity factory
// -------------------------------------------------------

void EditorLayer::CreateEntity(int meshType)
{
    static int count = 0;
    const char* names[]  = { "Cube", "Sphere", "Plane", "Cylinder" };
    glm::vec4   colors[] = {
        { 0.8f,  0.35f, 0.2f,  1 },
        { 0.2f,  0.55f, 0.85f, 1 },
        { 0.55f, 0.55f, 0.55f, 1 },
        { 0.75f, 0.65f, 0.25f, 1 }
    };

    std::shared_ptr<Pondo::Mesh> mesh;
    switch (meshType) {
    case 1:  mesh = Pondo::Mesh::CreateSphere();    break;
    case 2:  mesh = Pondo::Mesh::CreatePlane(1.0f); break;
    case 3:  mesh = Pondo::Mesh::CreateCylinder();  break;
    default: mesh = Pondo::Mesh::CreateCube();
    }

    auto* e = m_SceneLayer->SpawnEntity(
        std::string(names[meshType]) + std::to_string(++count),
        std::move(mesh), colors[meshType], { 0, 0.5f, 0 });
    m_SceneLayer->SetSelectedEntity(e);
}
