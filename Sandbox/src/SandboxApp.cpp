#define PD_ENTRY_POINT
#include <Pondo.h>

#include "Pondo/Scene/SceneLayer.h"
#include "Pondo/Scene/EditorLayer.h"

// -------------------------------------------------------
//  Sandbox application
//  Creates the two layers and pushes them onto the stack.
//  All logic lives in SceneLayer and EditorLayer.
// -------------------------------------------------------

class Sandbox : public Pondo::Application
{
public:
    Sandbox()
    {
        Pondo::Application::InitGlad();
        m_SceneLayer = new SceneLayer();
        m_EditorLayer = new EditorLayer(m_SceneLayer);
        PushLayer(m_SceneLayer);
        PushOverlay(m_EditorLayer);
    }

    ~Sandbox()
    {
        delete m_SceneLayer;
        delete m_EditorLayer;
    }

private:
    SceneLayer* m_SceneLayer = nullptr;
    EditorLayer* m_EditorLayer = nullptr;
};

Pondo::Application* Pondo::CreateApplication() { return new Sandbox(); }