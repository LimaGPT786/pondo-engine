#pragma once

#ifdef PD_PLATFORM_WINDOWS

extern Pondo::Application* Pondo::CreateApplication();

int main(int argc, char** argv) {
	printf("Pondo Engine\n");

	auto app = Pondo::CreateApplication();
	app->Run();
	delete app;
}

#endif