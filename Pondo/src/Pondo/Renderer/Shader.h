#pragma once

#include "../Core.h"
#include <string>
#include <glm/glm.hpp>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)
	class PONDO_API Shader {
	public:
		Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
		~Shader();

		void Bind() const;
		void Unbind() const;

		void SetMat4(const std::string& name, const glm::mat4& matrix) const;
		void SetFloat3(const std::string& name, const glm::vec3& vec) const;
		void SetFloat4(const std::string& name, const glm::vec4& vec) const;
		void SetInt(const std::string& name, int value) const;

	private:
		unsigned int m_RendererID;
	};
#pragma warning(pop)

}