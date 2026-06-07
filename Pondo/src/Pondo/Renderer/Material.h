#pragma once

#include "../Core.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)
	class PONDO_API Material {
	public:
		Material(const std::shared_ptr<Shader>& shader);

		void Bind() const;

		// Convenience setters — just forwarded to the shader
		void SetColor(const glm::vec4& color) { m_Color = color; }
		void SetMetallic(float v) { m_Metallic = v; }
		void SetRoughness(float v) { m_Roughness = v; }

		const glm::vec4& GetColor()    const { return m_Color; }
		float            GetMetallic() const { return m_Metallic; }
		float            GetRoughness()const { return m_Roughness; }

		const std::shared_ptr<Shader>& GetShader() const { return m_Shader; }

	private:
		std::shared_ptr<Shader> m_Shader;
		glm::vec4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		float     m_Metallic = 0.0f;
		float     m_Roughness = 0.5f;
	};
#pragma warning(pop)

}