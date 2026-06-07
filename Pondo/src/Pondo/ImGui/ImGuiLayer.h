#pragma once

#include "../Layer.h"

namespace Pondo {

	class PONDO_API ImGuiLayer : public Layer {
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		void OnAttach() override {}
		void OnDetach() override {}
		void OnEvent(Event& e) override;

		void BlockEvents(bool block) { m_BlockEvents = block; }

	private:
		bool m_BlockEvents = true;
	};

}