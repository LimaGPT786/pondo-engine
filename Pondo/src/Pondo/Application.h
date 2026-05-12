#pragma once

#include "Core.h"

namespace Pondo {
	class PONDO_API Application {
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}