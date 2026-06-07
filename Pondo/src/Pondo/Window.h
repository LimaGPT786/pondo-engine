#pragma once
#include "Core.h"
#include "Events/Event.h"
#include <string>
#include <functional>

struct GLFWwindow;

namespace Pondo {

	struct WindowConfig {
		std::string Title = "Pondo Engine";
		int Width = 1280;
		int Height = 720;
		bool startWindowedFullScreen = false;
	};

	class PONDO_API Window {
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		Window(const WindowConfig& config = WindowConfig{});
		~Window();

		bool Init();
		bool ShouldClose() const;
		void OnUpdate();
		void Shutdown();

		void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

		int GetWidth()  const { return m_Data.Width; }
		int GetHeight() const { return m_Data.Height; }

		void* GetNativeWindow() const { return m_Window; }

	private:
		struct WindowData {
			std::string Title;
			int Width, Height;
			EventCallbackFn EventCallback;
		};

		GLFWwindow* m_Window;
		WindowConfig m_Config;
		WindowData   m_Data;
	};

}
