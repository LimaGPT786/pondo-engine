#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Timestep.h"

#include <string>

namespace Engine {

    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        // Called when the layer is pushed onto the stack.
        // Use this for initialization instead of the constructor —
        // the renderer and other systems are guaranteed to be ready here.
        virtual void OnAttach() {}

        // Called when the layer is popped or the application shuts down.
        virtual void OnDetach() {}

        // Called every frame — put per-frame logic here.
        virtual void OnUpdate(Timestep ts) {}

        // Called every frame after OnUpdate for ImGui rendering.
        virtual void OnImGuiRender() {}

        // Called when an event is dispatched. Set e.Handled = true to stop propagation.
        virtual void OnEvent(Event& e) {}

        const std::string& GetName() const { return m_DebugName; }

    protected:
        std::string m_DebugName;
    };

} // namespace Engine
