#pragma once

#include "../Core.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Material.h"
#include "LightEnvironment.h"
#include <memory>

namespace Pondo {

    class PONDO_API Renderer {
    public:
        struct SceneData {
            glm::mat4        ViewProjectionMatrix;
            glm::vec3        CameraPosition;
            LightEnvironment Lights;
        };
        // Pass the light environment alongside the camera so the
        // renderer can push all uniforms before any mesh draws.
        static void BeginScene(const Camera& camera,
                               const LightEnvironment& lights = LightEnvironment{});
        static void EndScene();

        // Legacy path (keeps the old triangle demo working)
        static void Submit(const std::shared_ptr<Shader>& shader,
                           const std::shared_ptr<VertexArray>& vertexArray);

        // Mesh path
        static void SubmitMesh(const std::shared_ptr<Mesh>& mesh,
                               const std::shared_ptr<Material>& material,
                               const glm::mat4& transform = glm::mat4(1.0f));

        static void SetClearColor(float r, float g, float b, float a);
        static void Clear();

    private:
        static SceneData* s_SceneData;
    };

} // namespace Pondo
