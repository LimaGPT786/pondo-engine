#pragma once

// ---------------------------------------------------------------
// Application.h  (relevant excerpt — wire LayerStack into your
// existing Application class like this)
// ---------------------------------------------------------------

#include "LayerStack.h"
#include "Timestep.h"
#include "Window.h"           // your existing GLFW window wrapper
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "ImGui/ImGuiLayer.h" // add after you build the ImGui layer

namespace Engine {

    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        void Close() { m_Running = false; }

        void OnEvent(Event& e);

        void PushLayer(Layer* layer)   { m_LayerStack.PushLayer(layer); }
        void PushOverlay(Layer* layer) { m_LayerStack.PushOverlay(layer); }

        static Application& Get() { return *s_Instance; }
        Window& GetWindow()       { return *m_Window; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

    private:
        std::unique_ptr<Window> m_Window;
        ImGuiLayer* m_ImGuiLayer;   // kept for direct ImGui frame calls
        LayerStack  m_LayerStack;

        bool  m_Running   = true;
        bool  m_Minimized = false;
        float m_LastFrameTime = 0.0f;

        static Application* s_Instance;
    };

    // Defined by the CLIENT (your game / editor project)
    Application* CreateApplication();

} // namespace Engine


// ---------------------------------------------------------------
// Application.cpp  (run-loop wiring)
// ---------------------------------------------------------------
/*

void Application::Run()
{
    while (m_Running)
    {
        // --- Timestep -------------------------------------------
        float time      = (float)glfwGetTime();        // seconds
        Timestep ts     = time - m_LastFrameTime;
        m_LastFrameTime = time;

        // --- Per-frame layer updates (front → back) -------------
        if (!m_Minimized)
        {
            for (Layer* layer : m_LayerStack)
                layer->OnUpdate(ts);
        }

        // --- ImGui render pass (overlay, always last) -----------
        m_ImGuiLayer->Begin();
        for (Layer* layer : m_LayerStack)
            layer->OnImGuiRender();
        m_ImGuiLayer->End();

        // --- Platform tick / swap buffers -----------------------
        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(
        ENGINE_BIND_EVENT_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(
        ENGINE_BIND_EVENT_FN(Application::OnWindowResize));

    // Propagate to layers BACK → FRONT so overlays (ImGui) get first pick.
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
        if (e.Handled)
            break;
        (*it)->OnEvent(e);
    }
}

*/
