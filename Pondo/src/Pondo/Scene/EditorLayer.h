#pragma once

#include <Pondo.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>

#include "SceneLayer.h"
#include "EditorCommands.h"

// -------------------------------------------------------
//  EditorLayer
//  Pure manual ImGuiIO — zero GLFW backend.
//  Owns all ImGui panels: menu bar, scene panel,
//  properties panel, and viewport.
// -------------------------------------------------------

#pragma warning(push)
#pragma warning(disable: 4251)
class PONDO_API EditorLayer : public Pondo::Layer
{
public:
    explicit EditorLayer(SceneLayer* sl);

    EditorLayer(const EditorLayer&) = delete;
    EditorLayer& operator=(const EditorLayer&) = delete;

    void OnAttach() override;
    void OnDetach() override;
    void OnRender() override;
    void OnEvent(Pondo::Event& e) override;

    // Public so the Viewport section in OnRender can reach it
    std::string m_StatusText;
    float       m_StatusTimer = 0.0f;

private:
    // --- Command history ---
    void Execute(std::unique_ptr<ICommand> cmd);
    void Undo();
    void Redo();

    // --- Entity factory (called from menu / scene panel) ---
    void CreateEntity(int meshType);

    // --- State ---
    GLFWwindow* m_Window = nullptr;
    SceneLayer* m_SceneLayer = nullptr;
    float        m_LastTime = 0.0f;

    std::shared_ptr<Pondo::Framebuffer> m_Framebuffer;
    std::string  m_CurrentScenePath;

    bool m_ShowScene = true;
    bool m_ShowProps = true;
    bool m_ShowStats = true;
    bool m_ShowLighting = true;

    bool  m_EnableSnapping = false;
    float m_MoveIncrement = 0.5f;
    float m_RotateIncrement = 15.0f;
    float m_ScaleIncrement = 0.1f;

    std::vector<std::unique_ptr<ICommand>> m_History;
    int  m_HistoryIndex = -1;

    bool m_WasEditingTransform = false;
    std::vector<Pondo::TransformComponent> m_TransformsBefore;

    bool        m_ShowUnionError = false;
    std::string m_UnionErrorMsg;
};
#pragma warning(pop)
