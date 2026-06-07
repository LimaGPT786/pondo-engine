#include "Renderer.h"
#include <glad/glad.h>

namespace Pondo {

	Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData();

	void Renderer::BeginScene(const Camera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjection();
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
		material->Bind();
		material->GetShader()->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		material->GetShader()->SetMat4("u_Transform", transform);

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

}