#include <Pondo.h>

class Sandbox : public Pondo::Application {
public:
	Sandbox() {

	}

	~Sandbox() {

	}
};

Pondo::Application* Pondo::CreateApplication() {
	return new Sandbox();
}