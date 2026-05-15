#include "Application.h"

namespace Pondo {
    Application::Application()
    {
        m_window = new Window({ "Pondo Engine", 1280, 720, false });
        m_window->Init();
    }

    Application::~Application()
    {
        delete m_window;
    }

    void Application::Run()
    {
        while (!m_window->ShouldClose())
        {
            m_window->OnUpdate();
        }
    }
}