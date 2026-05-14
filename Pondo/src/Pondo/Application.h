#pragma once

#include "Core.h"

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
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}