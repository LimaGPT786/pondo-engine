#pragma once

#include "Core.h"
#include "Layer.h"

#include <vector>

namespace Engine {

    // LayerStack owns all Layer pointers and manages their lifetime.
    //
    // Internal layout of m_Layers:
    //
    //   [ Layer, Layer, Layer, | Overlay, Overlay ]
    //                          ^
    //                    m_LayerInsert
    //
    // PushLayer  — inserts before m_LayerInsert (regular layers, rendered first).
    // PushOverlay — appends after  m_LayerInsert (overlays like ImGui, rendered last).
    //
    // Update iterates FRONT → BACK  (layers get ticked before overlays).
    // Events  iterate BACK  → FRONT (overlays consume first, e.g. ImGui blocks clicks).

    class LayerStack
    {
    public:
        LayerStack();
        ~LayerStack();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        // Range iterators — used by Application to call OnUpdate and OnEvent.
        std::vector<Layer*>::iterator       begin()  { return m_Layers.begin(); }
        std::vector<Layer*>::iterator       end()    { return m_Layers.end(); }
        std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
        std::vector<Layer*>::reverse_iterator rend()   { return m_Layers.rend(); }

        // Const versions for read-only access.
        std::vector<Layer*>::const_iterator       begin()  const { return m_Layers.begin(); }
        std::vector<Layer*>::const_iterator       end()    const { return m_Layers.end(); }
        std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
        std::vector<Layer*>::const_reverse_iterator rend()   const { return m_Layers.rend(); }

    private:
        std::vector<Layer*> m_Layers;
        unsigned int m_LayerInsertIndex = 0; // index-based to survive reallocation
    };

} // namespace Engine
