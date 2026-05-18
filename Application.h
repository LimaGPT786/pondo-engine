#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvents.h"
#include "Events/Event.h"
#include "LayerStack.h"
#include "Timestep.h"

namespace Pondo {
	/**
	 * The class that deals with application initialization, lifetime, and shutdown.
	 * Any Pondo game will create a class that inherits from this, and implement the 
	 * CreateApplication() method in their own code (Sandbox) to return a new instance 
	 * of their class.
	 */
	class PONDO_API Application {
	public:
		Application();
		virtual ~Application();
		void Run();
		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		Window* m_Window;
		LayerStack m_LayerStack;

		bool  m_Running   = true;
		bool  m_Minimized = false;
		float m_LastFrameTime = 0.0f;

		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}
