#pragma once

#include "Core.h"
#include "Layer.h"
#include <vector>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)
	class PONDO_API LayerStack {
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator         begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator         end() { return m_Layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

	private:
		std::vector<Layer*> m_Layers;
		unsigned int        m_LayerInsertIndex = 0;
	};
#pragma warning(pop)

}