#pragma once

#ifdef PD_PLATFORM_WINDOWS

/* Leave creating the application over to the client (sandbox) */
extern Pondo::Application* Pondo::CreateApplication();

int main(int argc, char** argv) {
	Pondo::Log::Init();

	int a = 5;

	PD_CORE_WARN("Initialized Log!");
	PD_INFO("Hello! Var={0}", a);

	auto app = Pondo::CreateApplication();
	app->Run();
	delete app;
}

#endif