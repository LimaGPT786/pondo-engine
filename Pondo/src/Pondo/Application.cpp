#include "Application.h"
#include "Log.h"

namespace Pondo {
    Application::Application()
    {
        m_window = new Window({ "Pondo Engine", 1280, 720, false });
        m_window->Init();
        m_window->SetEventCallback([this](Event& e) { OnEvent(e); });
    }

    Application::~Application()
    {
        delete m_window;
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) {
            return OnWindowClose(e);
            }
        );

        PD_CORE_TRACE("{0}", e.ToString());
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_running = false;
        return true;
    }

    void Application::Run()
    {
        while (m_running)
        {
            m_window->OnUpdate();
        }
    }
}