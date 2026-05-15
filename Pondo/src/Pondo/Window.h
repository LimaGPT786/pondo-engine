#pragma once
#include "Core.h"
#include <string>

struct GLFWwindow;  // forward-declare so header doesn't need glfw3.h

namespace Pondo {

    struct WindowConfig {
        std::string Title = "Pondo Engine";
        int Width  = 1280;
        int Height = 720;
        bool startWindowedFullScreen = false;
    };

    class PONDO_API Window {
    public:
        Window(const WindowConfig& config = WindowConfig{});
        ~Window();

        bool Init();
        bool ShouldClose() const;
        void OnUpdate();
        void Shutdown();

    private:
        GLFWwindow*  m_Window;
        WindowConfig m_Config;
    };

} // namespace Pondo