#include "Window.h"
#include "Events/ApplicationEvents.h"
#include "Events/KeyEvents.h"
#include "Events/MouseEvents.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Pondo {

    Window::Window(const WindowConfig& config)
        : m_Config(config), m_Window(nullptr)
    {
    }

    Window::~Window() { Shutdown(); }

    bool Window::Init()
    {
        if (!glfwInit())
            return false;

        m_Window = glfwCreateWindow(
            m_Config.Width, m_Config.Height,
            m_Config.Title.c_str(),
            nullptr, nullptr);

        if (!m_Window) { glfwTerminate(); return false; }

        glfwMakeContextCurrent(m_Window);

        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        PD_CORE_ASSERT(status, "Failed to initialize glad!");

        m_Data.Title = m_Config.Title;
        m_Data.Width = m_Config.Width;
        m_Data.Height = m_Config.Height;

        glfwSetWindowUserPointer(m_Window, &m_Data);

        // Window close
        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* w) {
            auto& data = *(WindowData*)glfwGetWindowUserPointer(w);
            WindowCloseEvent event;
            data.EventCallback(event);
            });

        // Window resize
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* w, int width, int height) {
            auto& data = *(WindowData*)glfwGetWindowUserPointer(w);
            data.Width = width;
            data.Height = height;
            glViewport(0, 0, width, height);
            WindowResizeEvent event(width, height);
            data.EventCallback(event);
            });

        // Key
        glfwSetKeyCallback(m_Window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
            auto& data = *(WindowData*)glfwGetWindowUserPointer(w);
            switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent event(key, 0);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent event(key);
                data.EventCallback(event);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent event(key, 1);
                data.EventCallback(event);
                break;
            }
            }
            });

        // Char/typed
        glfwSetCharCallback(m_Window, [](GLFWwindow* w, unsigned int keycode) {
            auto& data = *(WindowData*)glfwGetWindowUserPointer(w);
            KeyTypedEvent event(keycode);
            data.EventCallback(event);
            });

        // Mouse button
        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* w, int button, int action, int mods) {
            auto& data = *(WindowData*)glfwGetWindowUserPointer(w);
            switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent event(button);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent event(button);
                data.EventCallback(event);
                break;
            }
            }
            });

        // Mouse scroll
        glfwSetScrollCallback(m_Window, [](GLFWwindow* w, double xOffset, double yOffset) {
            auto& data = *(WindowData*)glfwGetWindowUserPointer(w);
            MouseScrolledEvent event((float)xOffset, (float)yOffset);
            data.EventCallback(event);
            });

        // Mouse move
        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* w, double xPos, double yPos) {
            auto& data = *(WindowData*)glfwGetWindowUserPointer(w);
            MouseMovedEvent event((float)xPos, (float)yPos);
            data.EventCallback(event);
            });

        return true;
    }

    bool Window::ShouldClose() const { return glfwWindowShouldClose(m_Window); }

    void Window::OnUpdate()
    {
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }

    void Window::Shutdown()
    {
        if (m_Window) {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
        glfwTerminate();
    }

}
