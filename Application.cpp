#include "Application.h"
#include "Log.h"

#include <GLFW/glfw3.h>

namespace Pondo {

    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        PD_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        m_Window = new Window({ "Pondo Engine", 1280, 720, false });
        m_Window->Init();
        m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });
    }

    Application::~Application()
    {
        delete m_Window;
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Layer* overlay)
    {
        m_LayerStack.PushOverlay(overlay);
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) {
            return OnWindowClose(e);
        });
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) {
            return OnWindowResize(e);
        });

        PD_CORE_TRACE("{0}", e.ToString());

        // Propagate to layers back → front so overlays (e.g. ImGui) consume first.
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.Handled)
                break;
            (*it)->OnEvent(e);
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;
        return false; // let layers hear it too (e.g. camera needs to update aspect ratio)
    }

    void Application::Run()
    {
        while (m_Running)
        {
            // --- Timestep ---------------------------------------------------
            float time        = (float)glfwGetTime(); // seconds since init
            Timestep ts       = time - m_LastFrameTime;
            m_LastFrameTime   = time;

            // --- Update all layers (front → back) ---------------------------
            if (!m_Minimized)
            {
                for (Layer* layer : m_LayerStack)
                    layer->OnUpdate(ts);
            }

            // --- ImGui render pass (every layer contributes) ----------------
            // Uncomment once ImGuiLayer is added:
            // m_ImGuiLayer->Begin();
            // for (Layer* layer : m_LayerStack)
            //     layer->OnImGuiRender();
            // m_ImGuiLayer->End();

            // --- Platform tick / swap buffers -------------------------------
            m_Window->OnUpdate();
        }
    }

} // namespace Pondo
