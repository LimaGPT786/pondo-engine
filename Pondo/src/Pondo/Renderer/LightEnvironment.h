#pragma once

#include "../Core.h"
#include <glm/glm.hpp>
#include <array>

namespace Pondo {

    static constexpr int MAX_POINT_LIGHTS = 8;
    static constexpr int MAX_SPOT_LIGHTS  = 4;

    // ---- per-light POD structs ------------------------------------

    struct DirectionalLightData
    {
        glm::vec3 Direction = { -0.4f, -1.0f, -0.3f };
        glm::vec3 Color     = {  1.0f,  1.0f,  1.0f };
        float     Intensity = 1.0f;
        bool      Active    = true;
    };

    struct PointLightData
    {
        glm::vec3 Position = { 0,0,0 };
        glm::vec3 Color = { 1,1,1 };

        float Intensity = 1.0f;
        float Range = 20.0f;

        float Constant = 1.0f;
        float Linear = 0.09f;
        float Quadratic = 0.032f;
    };

    struct SpotLightData
    {
        glm::vec3 Position        = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Direction       = { 0.0f, -1.0f, 0.0f };
        glm::vec3 Color           = { 1.0f,  1.0f, 1.0f };
        float     Intensity       = 1.0f;
        float     Range           = 15.0f;
        float     Constant        = 1.0f;
        float     Linear          = 0.09f;
        float     Quadratic       = 0.032f;
        float     InnerCutoffCos  = 0.9763f;  // cos(12.5 deg)
        float     OuterCutoffCos  = 0.9563f;  // cos(17.5 deg)
    };

    // ---- what Renderer::BeginScene receives each frame ------------

    struct LightEnvironment
    {
        // Global ambient (like Roblox's Lighting.Ambient)
        glm::vec3 AmbientColor     = { 0.4f, 0.4f, 0.5f };
        float     AmbientIntensity = 0.3f;

        // Sun / sky (like Roblox's ColorCorrectionEffect + sun angle)
        DirectionalLightData Sun;

        // Placeable lights
        int                                          PointLightCount = 0;
        std::array<PointLightData,  MAX_POINT_LIGHTS> PointLights;

        int                                          SpotLightCount  = 0;
        std::array<SpotLightData,   MAX_SPOT_LIGHTS>  SpotLights;

        // Call at the start of each frame before collecting lights
        void Clear()
        {
            AmbientColor     = { 0.4f, 0.4f, 0.5f };
            AmbientIntensity = 0.3f;
            Sun.Active       = false;
            PointLightCount  = 0;
            SpotLightCount   = 0;
        }
    };

} // namespace Pondo
