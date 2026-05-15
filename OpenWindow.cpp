#include "Window.h"
#include <GLFW/glfw3.h>

namespace Pondo {

    Window::Window(const WindowConfig& config)
        : m_Config(config), m_Window(nullptr)
    {
    }

    Window::~Window()
    {
        Shutdown();
    }

    bool Window::Init()
    {
        if (!glfwInit())
            return false;

        if (m_Config.startWindowedFullScreen)
        {
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

            glfwWindowHint(GLFW_RED_BITS,     mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS,   mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS,    mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            m_Window = glfwCreateWindow(mode->width, mode->height, m_Config.Title.c_str(), glfwGetPrimaryMonitor(), nullptr);

            glfwGetWindowSize(m_Window, &m_Config.Width, &m_Config.Height);
        }
        else
        {
            m_Window = glfwCreateWindow(m_Config.Width, m_Config.Height, m_Config.Title.c_str(), nullptr, nullptr);
        }

        if (!m_Window)
        {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_Window);
        return true;
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }

    void Window::OnUpdate()
    {
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }

    void Window::Shutdown()
    {
        if (m_Window)
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
        glfwTerminate();
    }

} // namespace Pondo