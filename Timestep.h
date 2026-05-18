#pragma once

namespace Engine {

    // Wraps a frame delta-time. Stored in seconds internally;
    // call GetMilliseconds() when you need ms (e.g. for display).
    class Timestep
    {
    public:
        Timestep(float time = 0.0f)
            : m_Time(time)
        {
        }

        // Implicit conversion so you can write:  float dt = ts;
        operator float() const { return m_Time; }

        float GetSeconds()      const { return m_Time; }
        float GetMilliseconds() const { return m_Time * 1000.0f; }

    private:
        float m_Time; // seconds
    };

} // namespace Engine
