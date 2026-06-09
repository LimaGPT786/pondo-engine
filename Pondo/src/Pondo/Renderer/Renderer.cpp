#include "Renderer.h"
#include <glad/glad.h>

namespace Pondo {

    Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData();

    // ---- helpers (keep the Submit call in SubmitMesh DRY) ----------

    static void UploadLights(const std::shared_ptr<Shader>& shader,
        const Renderer::SceneData* data)
    {
        const LightEnvironment& L = data->Lights;

        // camera
        shader->SetFloat3(
            "u_CameraPos",
            data->CameraPosition);

        // ambient
        shader->SetFloat3(
            "u_AmbientColor",
            L.AmbientColor);

        shader->SetFloat(
            "u_AmbientIntensity",
            L.AmbientIntensity);

        // sun
        shader->SetInt(
            "u_SunActive",
            L.Sun.Active ? 1 : 0);

        shader->SetFloat3(
            "u_SunDirection",
            L.Sun.Direction);

        shader->SetFloat3(
            "u_SunColor",
            L.Sun.Color);

        shader->SetFloat(
            "u_SunIntensity",
            L.Sun.Intensity);

        // ---------------------------------
        // Point lights
        // ---------------------------------

        shader->SetInt(
            "u_PointLightCount",
            L.PointLightCount);

        for (int i = 0; i < L.PointLightCount; i++)
        {
            const auto& pl =
                L.PointLights[i];

            shader->SetFloat3(
                "u_PointLightPos[" +
                std::to_string(i) +
                "]",
                pl.Position);

            shader->SetFloat3(
                "u_PointLightColor[" +
                std::to_string(i) +
                "]",
                pl.Color);

            shader->SetFloat(
                "u_PointLightIntensity[" +
                std::to_string(i) +
                "]",
                pl.Intensity);

            shader->SetFloat(
                "u_PointLightRange[" +
                std::to_string(i) +
                "]",
                pl.Range);
        }

        // ---------------------------------
        // Spot lights
        // ---------------------------------

        shader->SetInt(
            "u_SpotLightCount",
            L.SpotLightCount);

        for (int i = 0; i < L.SpotLightCount; i++)
        {
            const auto& sl =
                L.SpotLights[i];

            shader->SetFloat3(
                "u_SpotLightPos[" +
                std::to_string(i) +
                "]",
                sl.Position);

            shader->SetFloat3(
                "u_SpotLightDir[" +
                std::to_string(i) +
                "]",
                sl.Direction);

            shader->SetFloat3(
                "u_SpotLightColor[" +
                std::to_string(i) +
                "]",
                sl.Color);

            shader->SetFloat(
                "u_SpotLightIntensity[" +
                std::to_string(i) +
                "]",
                sl.Intensity);

            shader->SetFloat(
                "u_SpotLightInnerCos[" +
                std::to_string(i) +
                "]",
                sl.InnerCutoffCos);

            shader->SetFloat(
                "u_SpotLightOuterCos[" +
                std::to_string(i) +
                "]",
                sl.OuterCutoffCos);
        }
    }

    // ----------------------------------------------------------------

    void Renderer::BeginScene(const Camera& camera, const LightEnvironment& lights)
    {
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjection();
        s_SceneData->CameraPosition       = camera.GetPosition();
        s_SceneData->Lights               = lights;
    }

    void Renderer::EndScene() {}

    void Renderer::Submit(const std::shared_ptr<Shader>& shader,
                          const std::shared_ptr<VertexArray>& vertexArray)
    {
        shader->Bind();
        shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
        vertexArray->Bind();

        if (vertexArray->GetIndexBuffer())
            glDrawElements(GL_TRIANGLES,
                           vertexArray->GetIndexBuffer()->GetCount(),
                           GL_UNSIGNED_INT, nullptr);
        else
            glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void Renderer::SubmitMesh(const std::shared_ptr<Mesh>& mesh,
                              const std::shared_ptr<Material>& material,
                              const glm::mat4& transform)
    {
        const auto& shader = material->GetShader();

        material->Bind();
        shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
        shader->SetMat4("u_Transform",      transform);
        shader->SetFloat3("u_CameraPos", s_SceneData->CameraPosition);

        UploadLights(shader, s_SceneData);

        mesh->Bind();
        glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
        mesh->Unbind();
    }

    void Renderer::SetClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void Renderer::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

} // namespace Pondo
