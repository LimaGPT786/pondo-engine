#pragma once

#include "Core.h"
#include "Timestep.h"
#include "Events/Event.h"
#include <string>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)
	class PONDO_API Layer {
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnRender() {}
		virtual void OnEvent(Event& e) {}

		const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;
	};
#pragma warning(pop)

}