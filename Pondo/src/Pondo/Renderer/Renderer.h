#pragma once

#include "../Core.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Material.h"
#include <memory>

namespace Pondo {

	class PONDO_API Renderer {
	public:
		static void BeginScene(const Camera& camera);
		static void EndScene();

		// Legacy path (triangle demo still works)
		static void Submit(const std::shared_ptr<Shader>& shader,
			const std::shared_ptr<VertexArray>& vertexArray);

		// New mesh path
		static void SubmitMesh(const std::shared_ptr<Mesh>& mesh,
			const std::shared_ptr<Material>& material,
			const glm::mat4& transform = glm::mat4(1.0f));

		static void SetClearColor(float r, float g, float b, float a);
		static void Clear();

	private:
		struct SceneData {
			glm::mat4 ViewProjectionMatrix;
		};
		static SceneData* s_SceneData;
	};

}