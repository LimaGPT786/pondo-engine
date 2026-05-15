#include "Application.h"
#include "Window.h"

namespace Pondo {

    Application::Application()
    {
        m_Window = new Window({"Pondo Engine", 1280, 720, false});
        m_Window->Init();
    }

    Application::~Application()
    {
        delete m_Window;
    }

    void Application::Run()
    {
        while (!m_Window->ShouldClose())
        {
            m_Window->OnUpdate();
        }
    }

}