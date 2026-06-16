#include "EditorLayer.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <mcut/mcut.h>

#include "Pondo/ImGui/ImGuiKeyMap.h"
#include "FileDialogs.h"
#include "Pondo/Scene/SceneSerializer.h"
#include "Pondo/Events/ApplicationEvents.h"
#include "Pondo/Events/KeyEvents.h"
#include "Pondo/Events/MouseEvents.h"

#include <unordered_set>

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
    spec.Width = (unsigned)w;
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
    s.FrameRounding = 3.0f;
    s.Colors[ImGuiCol_WindowBg] = { 0.13f, 0.13f, 0.14f, 1 };
    s.Colors[ImGuiCol_TitleBg] = { 0.10f, 0.10f, 0.11f, 1 };
    s.Colors[ImGuiCol_TitleBgActive] = { 0.16f, 0.29f, 0.48f, 1 };
    s.Colors[ImGuiCol_Header] = { 0.20f, 0.35f, 0.55f, 0.6f };
    s.Colors[ImGuiCol_HeaderHovered] = { 0.26f, 0.47f, 0.70f, 0.8f };
    s.Colors[ImGuiCol_FrameBg] = { 0.18f, 0.18f, 0.20f, 1 };
    s.Colors[ImGuiCol_Button] = { 0.20f, 0.35f, 0.55f, 1 };
    s.Colors[ImGuiCol_ButtonHovered] = { 0.26f, 0.47f, 0.70f, 1 };

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
        ImGui::SetNextWindowPos(
            { displaySize.x - PAD, displaySize.y - PAD },
            ImGuiCond_Always,
            { 1.0f, 1.0f });

        ImGui::Begin(
            "##TransformSpace", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
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
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
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
            ImGui::MenuItem("Scene", nullptr, &m_ShowScene);
            ImGui::MenuItem("Properties", nullptr, &m_ShowProps);
            ImGui::MenuItem("Stats", nullptr, &m_ShowStats);
            ImGui::MenuItem("Lighting", nullptr, &m_ShowLighting);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Cube"))     CreateEntity(0);
            if (ImGui::MenuItem("Sphere"))   CreateEntity(1);
            if (ImGui::MenuItem("Plane"))    CreateEntity(2);
            if (ImGui::MenuItem("Cylinder")) CreateEntity(3);
            ImGui::Separator();
            if (ImGui::MenuItem("Point Light")) m_SceneLayer->SpawnLight("PointLight", Pondo::LightType::Point);
            if (ImGui::MenuItem("Spot Light"))  m_SceneLayer->SpawnLight("SpotLight", Pondo::LightType::Spot);
            ImGui::Separator();
            if (ImGui::MenuItem("Group Selected")) m_SceneLayer->CreateGroup("Group");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    float menuH = ImGui::GetWindowHeight();
    ImGui::End(); // ##Menu

    // ── Play + CSG toolbar ───────────────────────────────────────────
    auto playState = m_SceneLayer->GetPlayState();

    // Determine selection context for button enable states
    const auto& toolbarSel = m_SceneLayer->GetSelection();
    auto* toolbarPrimary = m_SceneLayer->GetSelectedEntity();
    bool        inEditMode = (playState == SceneLayer::PlayState::Edit);

    bool canUnion = true;
    bool canNegate = true;
    bool canSeparate = true;

    // Toolbar is centered and wide enough for Play + Stop + 3 CSG buttons
    const float toolbarW = 420.0f;
    ImGui::SetNextWindowPos({ (float)w * 0.5f - toolbarW * 0.5f, menuH });
    ImGui::SetNextWindowSize({ toolbarW, 36.0f });
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 6, 4 });
    ImGui::Begin("##Toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar(2);

    // ---- Play / Pause / Resume ----
    if (playState == SceneLayer::PlayState::Playing)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.6f, 0.0f, 1 });
        if (ImGui::Button("Pause", { 68, 24 })) m_SceneLayer->Pause();
        ImGui::PopStyleColor();
    }
    else if (playState == SceneLayer::PlayState::Paused)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.6f, 0.2f, 1 });
        if (ImGui::Button("Resume", { 68, 24 })) m_SceneLayer->Pause();
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.6f, 0.2f, 1 });
        if (ImGui::Button("Play", { 68, 24 })) m_SceneLayer->Play();
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    bool canStop = (playState != SceneLayer::PlayState::Edit);
    if (!canStop) ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Button,
        canStop ? ImVec4{ 0.7f, 0.15f, 0.15f, 1 } : ImVec4{ 0.35f, 0.35f, 0.35f, 1 });
    if (ImGui::Button("Stop", { 68, 24 })) m_SceneLayer->Stop();
    ImGui::PopStyleColor();
    if (!canStop) ImGui::EndDisabled();

    // ---- Separator ----
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // ---- Union ----
    ImGui::PushStyleColor(ImGuiCol_Button,
        canUnion ? ImVec4{ 0.18f, 0.38f, 0.65f, 1 } : ImVec4{ 0.25f, 0.25f, 0.25f, 1 });
    if (ImGui::Button("Union", { 72, 24 }) && canUnion)
    {
        m_SceneLayer->CreateGroup("Union");
        auto result = m_SceneLayer->UnionGroup();
        switch (result)
        {
        case SceneLayer::UnionResult::OK:
            m_StatusText = "Union applied successfully.";
            m_StatusTimer = 3.0f;
            break;
        case SceneLayer::UnionResult::PartialSuccess:
            m_UnionErrorMsg = "Union completed but one or more negate (subtract) operations failed.\nThe subtracted parts may not have intersected the positive mesh.";
            m_ShowUnionError = true;
            break;
        case SceneLayer::UnionResult::NoPositiveParts:
            m_UnionErrorMsg = "Union failed: all selected parts are set to Negate.\nAt least one part must NOT be negated to form the base mesh.";
            m_ShowUnionError = true;
            break;
        case SceneLayer::UnionResult::MeshOperationFailed:
            m_UnionErrorMsg = "Union failed: the mesh boolean operation returned an empty result.\nMake sure the meshes overlap and are valid (no zero-scale transforms).";
            m_ShowUnionError = true;
            break;
        case SceneLayer::UnionResult::NoChildren:
            m_UnionErrorMsg = "Union failed: the group has no mesh children.";
            m_ShowUnionError = true;
            break;
        default:
            break;
        }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // ---- Negate ----
    {
        // Tint button orange if any selected child is already negated (to show toggle)
        bool anyNegated = canNegate && std::any_of(toolbarSel.begin(), toolbarSel.end(),
            [](Pondo::Entity* e) { return e && e->InGroup() && e->IsNegate(); });
        ImGui::PushStyleColor(ImGuiCol_Button,
            !canNegate ? ImVec4{ 0.25f, 0.25f, 0.25f, 1 } :
            anyNegated ? ImVec4{ 0.65f, 0.30f, 0.10f, 1 } :
            ImVec4{ 0.50f, 0.22f, 0.08f, 1 });
        if (ImGui::Button("Negate", { 72, 24 }) && canNegate)
        {
            m_SceneLayer->NegateSelected();
            m_StatusText = "Negate toggled.";
            m_StatusTimer = 2.0f;
        }
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // ---- Separate ----
    ImGui::PushStyleColor(ImGuiCol_Button,
        canSeparate ? ImVec4{ 0.40f, 0.18f, 0.55f, 1 } : ImVec4{ 0.25f, 0.25f, 0.25f, 1 });
    if (ImGui::Button("Separate", { 80, 24 }) && canSeparate)
    {
        m_SceneLayer->UngroupSelected();
        m_StatusText = "Group separated.";
        m_StatusTimer = 2.0f;
    }
    ImGui::PopStyleColor();

    ImGui::End(); // ##Toolbar

    // ── Union error popup ─────────────────────────────────────────────
    if (m_ShowUnionError)
        ImGui::OpenPopup("Union Error##popup");

    ImGui::SetNextWindowSize({ 420, 0 }, ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Union Error##popup", nullptr, ImGuiWindowFlags_NoResize))
    {
        ImGui::TextWrapped("%s", m_UnionErrorMsg.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("OK", { 80, 0 }))
        {
            m_ShowUnionError = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    m_ShowUnionError = false; // reset flag each frame; OpenPopup keeps it alive

    // Layout constants — declared after toolbar so topY accounts for it
    const float sideW = 260.0f;
    float       topY = menuH + 36.0f;
    float       contentH = (float)h - topY;
    float leftW = m_ShowScene ? sideW : 0.0f;
    float rightW = (m_ShowProps || m_ShowStats) ? sideW : 0.0f;

    // ── Scene panel ──────────────────────────────────────────────────
    if (m_ShowScene)
    {
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
        // Draw entities — group roots as tree nodes, children indented under them
        std::unordered_set<uint32_t> drawnAsChild;

        for (auto& ep : m_SceneLayer->GetScene()->GetEntities())
        {
            auto* e = ep.get();
            if (e->InGroup()) { drawnAsChild.insert(e->GetID()); continue; } // drawn under parent

            if (e->IsGroupRoot())
            {
                auto children = m_SceneLayer->GetGroupChildren(e->GetID());
                bool anySelected = (m_SceneLayer->GetSelectedEntity() == e);
                ImGui::PushID((int)e->GetID());
                bool open = ImGui::TreeNodeEx(e->GetTag().c_str(),
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    (anySelected ? ImGuiTreeNodeFlags_Selected : 0));
                if (ImGui::IsItemClicked())
                {
                    // Select root + all children
                    m_SceneLayer->SetSelectedEntity(e);
                    for (auto* c : children)
                        m_SceneLayer->ToggleSelectedEntity(c);  // adds without clearing
                }
                if (open)
                {
                    for (auto* child : children)
                    {
                        bool csel = (m_SceneLayer->GetSelectedEntity() == child);

                        ImGui::PushID((int)child->GetID());

                        if (child->IsNegate())
                            ImGui::PushStyleColor(ImGuiCol_Text, { 1,0.4f,0.4f,1 });

                        if (ImGui::Selectable(child->GetTag().c_str(), csel))
                        {
                            bool shift =
                                Pondo::Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                                Pondo::Input::IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);

                            if (shift)
                                m_SceneLayer->ToggleSelectedEntity(child);
                            else
                                m_SceneLayer->SetSelectedEntity(child);
                        }

                        if (child->IsNegate())
                            ImGui::PopStyleColor();

                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            else
            {
                bool sel =
                    std::find(
                        m_SceneLayer->GetSelection().begin(),
                        m_SceneLayer->GetSelection().end(),
                        e)
                    !=
                    m_SceneLayer->GetSelection().end();
                ImGui::PushID((int)e->GetID());
                if (ImGui::Selectable(e->GetTag().c_str(), sel))
                    m_SceneLayer->SetSelectedEntity(e);
                ImGui::PopID();
            }
        }
        ImGui::Separator();
        if (ImGui::Button("+ Cube"))     CreateEntity(0); ImGui::SameLine();
        if (ImGui::Button("+ Sphere"))   CreateEntity(1); ImGui::SameLine();
        if (ImGui::Button("+ Plane"))    CreateEntity(2);
        if (ImGui::Button("+ Cylinder")) CreateEntity(3);

        if (m_SceneLayer->GetSelectedEntity()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.65f, 0.15f, 0.15f, 1 });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.80f, 0.20f, 0.20f, 1 });
            if (ImGui::Button("Delete Selected")) m_SceneLayer->DeleteSelected();
            ImGui::PopStyleColor(2);
        }
        ImGui::End();
    }

    // ── Lighting panel ───────────────────────────────────────────────
    if (m_ShowLighting)
    {
        float litY = m_ShowScene ? topY + contentH * 0.55f : topY;
        float litH = m_ShowScene ? contentH * 0.45f : contentH;

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
            ImGui::SliderFloat("Sun Intensity", &L.Sun.Intensity, 0.0f, 5.0f);
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
                ImGui::InputFloat("Move Step", &m_MoveIncrement, 0, 0, "%.3f");
                ImGui::InputFloat("Rotate Step", &m_RotateIncrement, 0, 0, "%.2f");
                ImGui::InputFloat("Scale Step", &m_ScaleIncrement, 0, 0, "%.3f");
                m_MoveIncrement = std::max(0.001f, m_MoveIncrement);
                m_RotateIncrement = std::max(0.01f, m_RotateIncrement);
                m_ScaleIncrement = std::max(0.001f, m_ScaleIncrement);

                ImGui::Separator();
                auto& tf = sel->GetTransform();

                if (ImGui::IsWindowFocused() && !m_WasEditingTransform) {
                    const auto& selection = m_SceneLayer->GetSelection();
                    m_TransformsBefore.clear();
                    for (auto* e : selection) m_TransformsBefore.push_back(e->GetTransform());
                }

                bool changed = false;
                changed |= ImGui::DragFloat3("Position", &tf.Position.x, 0.1f, 0.0f, 0.0f, "%.3f");
                changed |= ImGui::DragFloat3("Rotation", &tf.Rotation.x, 1.0f, 0.0f, 0.0f, "%.2f");
                changed |= ImGui::DragFloat3("Scale", &tf.Scale.x, 0.01f, 0.0f, 0.0f, "%.3f");
                tf.Scale.x = std::max(0.001f, tf.Scale.x);
                tf.Scale.y = std::max(0.001f, tf.Scale.y);
                tf.Scale.z = std::max(0.001f, tf.Scale.z);

                if (changed) m_WasEditingTransform = true;

                if (m_WasEditingTransform && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    const auto& selection = m_SceneLayer->GetSelection();
                    if (!m_TransformsBefore.empty()) {
                        if (selection.size() == 1) {
                            if (memcmp(&m_TransformsBefore[0], &tf, sizeof(tf)) != 0)
                                Execute(std::make_unique<TransformCommand>(selection[0], m_TransformsBefore[0], tf));
                        }
                        else {
                            std::vector<EntityTransformSnapshot> snaps;
                            for (size_t i = 0; i < selection.size() && i < m_TransformsBefore.size(); i++)
                                snaps.push_back({ selection[i], m_TransformsBefore[i], selection[i]->GetTransform() });
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
                        ImGui::InputFloat("Range", &lc->Range, 0.1f, 50.0f);
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
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.5f, 0.15f, 0.15f, 1 });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.7f, 0.20f, 0.20f, 1 });
                    if (ImGui::Button("Remove Light")) sel->RemoveLight();
                    ImGui::PopStyleColor(2);
                }
                else
                {
                    ImGui::Separator();
                    if (ImGui::Button("+ Point Light")) sel->AddLight(Pondo::LightType::Point);
                    ImGui::SameLine();
                    if (ImGui::Button("+ Spot Light"))  sel->AddLight(Pondo::LightType::Spot);
                }

                // ---- Script ----
                ImGui::Separator();
                ImGui::Text("Script");

                if (auto* sc = sel->GetScript())
                {
                    char scriptBuf[256];
                    strncpy_s(scriptBuf, sc->ScriptPath.c_str(), sizeof(scriptBuf));
                    if (ImGui::InputText("##scriptpath", scriptBuf, sizeof(scriptBuf)))
                        sel->SetScript(scriptBuf);
                    ImGui::SameLine();
                    if (ImGui::Button("...##script"))
                    {
                        std::string path = OpenFileDialog();
                        if (!path.empty()) sel->SetScript(path);
                    }
                    if (sc->HasError)
                        ImGui::TextColored({ 1, 0.3f, 0.3f, 1 }, "Error: %s", sc->ErrorMsg.c_str());
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.5f, 0.15f, 0.15f, 1 });
                    if (ImGui::Button("Remove Script")) sel->RemoveScript();
                    ImGui::PopStyleColor();
                }

                else
                {
                    if (ImGui::Button("+ Add Script"))
                        sel->SetScript("");
                }

                // ---- Group / Boolean ----
                ImGui::Separator();
                ImGui::Text("Model");

                auto& selection =
                    m_SceneLayer->GetSelection();

                if (selection.size() > 1)
                {
                    if (ImGui::Button("Union", { 100, 28 }))
                    {
                        m_SceneLayer->CreateGroup("Union");

                        auto* root =
                            m_SceneLayer->GetSelectedEntity();

                        if (root && root->IsGroupRoot())
                            m_SceneLayer->UnionGroup();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Negate", { 100, 28 }))
                    {
                        auto* root =
                            m_SceneLayer->GetSelectedEntity();

                        if (root && root->IsGroupRoot())
                        {
                            auto children =
                                m_SceneLayer->GetGroupChildren(
                                    root->GetID());

                            bool first = true;

                            for (auto* child : children)
                            {
                                if (first)
                                {
                                    first = false;
                                    continue;
                                }

                                child->SetGroup(
                                    root->GetID(),
                                    true);
                            }
                        }
                    }
                }

                if (sel->IsGroupRoot())
                {
                    ImGui::Separator();

                    if (ImGui::Button(
                        "Separate",
                        { 210, 28 }))
                    {
                        m_SceneLayer->UngroupSelected();
                    }

                    auto children =
                        m_SceneLayer->GetGroupChildren(
                            sel->GetID());

                    if (!children.empty())
                    {
                        ImGui::Separator();
                        ImGui::Text("Boolean Parts");

                        for (auto* child : children)
                        {
                            bool neg =
                                child->IsNegate();

                            if (ImGui::Checkbox(
                                child->GetTag().c_str(),
                                &neg))
                            {
                                child->SetGroup(
                                    sel->GetID(),
                                    neg);
                            }
                        }
                    }
                }
                else if (!sel->InGroup())
                {
                    if (ImGui::Button(
                        "+ Group Selected"))
                    {
                        m_SceneLayer->CreateGroup(
                            "Group");
                    }
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
    {
        float vpX = leftW;
        float vpW = (float)w - leftW - rightW;
        ImGui::SetNextWindowPos({ vpX, topY });
        ImGui::SetNextWindowSize({ vpW, contentH });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Viewport##panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar(2);

        ImVec2 contentPos = ImGui::GetCursorScreenPos();
        ImVec2 panelSize = ImGui::GetContentRegionAvail();

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

            // Gizmo / picking only in edit mode
            bool inEditMode = (playState == SceneLayer::PlayState::Edit);
            if (inEditMode)
            {
                glm::vec2 mouse = { io.MousePos.x, io.MousePos.y };
                bool overViewport = mouse.x >= contentPos.x && mouse.x < contentPos.x + panelSize.x &&
                    mouse.y >= contentPos.y && mouse.y < contentPos.y + panelSize.y;
                bool lmbDown = io.MouseDown[0];
                bool lmbClicked = io.MouseClicked[0];
                bool rmbDown = io.MouseDown[1];
                bool dragging = m_SceneLayer->IsDraggingGizmo();

                m_SceneLayer->SetViewportFocused(overViewport && !dragging);

                // Ctrl+B → Duplicate
                if (overViewport && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B, false)) {
                    Pondo::Entity* copy = m_SceneLayer->DuplicateEntity(m_SceneLayer->GetSelectedEntity());
                    if (copy) m_SceneLayer->SetSelectedEntity(copy);
                }

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
            else
            {
                // In play mode the viewport still needs focus for camera/input
                m_SceneLayer->SetViewportFocused(
                    io.MousePos.x >= contentPos.x && io.MousePos.x < contentPos.x + panelSize.x &&
                    io.MousePos.y >= contentPos.y && io.MousePos.y < contentPos.y + panelSize.y);
            }
        }

        // Status toast
        if (m_StatusTimer > 0.0f) {
            m_StatusTimer -= io.DeltaTime;
            ImGui::SetNextWindowBgAlpha(0.8f);
            ImGui::SetNextWindowPos({ (float)w * 0.5f - 60, topY + 8 });
            ImGui::Begin("##status", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove);
            ImGui::Text("%s", m_StatusText.c_str());
            ImGui::End();
        }

        ImGui::End(); // Viewport
    }

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
    m_StatusText = "Undo";
    m_StatusTimer = 1.0f;
}

void EditorLayer::Redo()
{
    if (m_HistoryIndex + 1 >= (int)m_History.size()) return;
    m_HistoryIndex++;
    m_History[m_HistoryIndex]->Redo();
    m_StatusText = "Redo";
    m_StatusTimer = 1.0f;
}

// -------------------------------------------------------
//  Entity factory
// -------------------------------------------------------

void EditorLayer::CreateEntity(int meshType)
{
    static int count = 0;
    const char* names[] = { "Cube", "Sphere", "Plane", "Cylinder" };
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