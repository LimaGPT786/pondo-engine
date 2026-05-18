#pragma once

#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvents.h"
#include "Events/Event.h"

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

	private:
		bool OnWindowClose(WindowCloseEvent& e);

		Window* m_window;
		bool m_running = true;
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}