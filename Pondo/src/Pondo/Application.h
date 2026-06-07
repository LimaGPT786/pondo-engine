#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Events/ApplicationEvents.h"
#include "Events/Event.h"

namespace Pondo {

	class PONDO_API Application {
	public:
		Application();
		virtual ~Application();

		void Run();
		void Close() { m_Running = false; }
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		Window& GetWindow() { return *m_Window; }

		static Application& Get() { return *s_Instance; }
		static void* GetProcAddress(const char* name);

	private:
		bool OnWindowClose(WindowCloseEvent& e);

		Window* m_Window;
		bool       m_Running = true;
		LayerStack m_LayerStack;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}
