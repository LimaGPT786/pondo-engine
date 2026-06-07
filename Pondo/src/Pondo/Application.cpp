#include "Application.h"
#include "Input.h"
#include "Log.h"
#include <GLFW/glfw3.h>

namespace Pondo {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		s_Instance = this;

		m_Window = new Window({ "Pondo Engine", 1280, 720, false });
		m_Window->Init();
		m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });
	}

	Application::~Application()
	{
		delete m_Window;
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
	}

	void* Application::GetProcAddress(const char* name)
	{
		return (void*)glfwGetProcAddress(name);
	}

	void Application::OnEvent(Event& e)
	{
		PD_CORE_TRACE("{0}", e.ToString());

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) {
			return OnWindowClose(e);
			});

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	void Application::Run()
	{
		float lastFrameTime = 0.0f;

		while (m_Running)
		{
			float time = (float)glfwGetTime();
			Timestep timestep = time - lastFrameTime;
			lastFrameTime = time;

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate(timestep);

			for (Layer* layer : m_LayerStack)
				layer->OnRender();

			m_Window->OnUpdate();
		}
	}

}
