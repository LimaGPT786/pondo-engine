#include "Shader.h"
#include "../Log.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Pondo {

	Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
		const char* src = vertexSrc.c_str();
		glShaderSource(vert, 1, &src, nullptr);
		glCompileShader(vert);

		int success;
		glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
		if (!success) {
			char log[512];
			glGetShaderInfoLog(vert, 512, nullptr, log);
			PD_CORE_ERROR("Vertex shader error: {0}", log);
		}

		unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
		src = fragmentSrc.c_str();
		glShaderSource(frag, 1, &src, nullptr);
		glCompileShader(frag);

		glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
		if (!success) {
			char log[512];
			glGetShaderInfoLog(frag, 512, nullptr, log);
			PD_CORE_ERROR("Fragment shader error: {0}", log);
		}

		m_RendererID = glCreateProgram();
		glAttachShader(m_RendererID, vert);
		glAttachShader(m_RendererID, frag);
		glLinkProgram(m_RendererID);

		glGetProgramiv(m_RendererID, GL_LINK_STATUS, &success);
		if (!success) {
			char log[512];
			glGetProgramInfoLog(m_RendererID, 512, nullptr, log);
			PD_CORE_ERROR("Shader link error: {0}", log);
		}

		glDeleteShader(vert);
		glDeleteShader(frag);
	}

	Shader::~Shader()
	{
		glDeleteProgram(m_RendererID);
	}

	void Shader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void Shader::Unbind() const
	{
		glUseProgram(0);
	}

	void Shader::SetMat4(const std::string& name, const glm::mat4& matrix) const
	{
		int location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void Shader::SetFloat3(const std::string& name, const glm::vec3& vec) const
	{
		int location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, vec.x, vec.y, vec.z);
	}

	void Shader::SetFloat4(const std::string& name, const glm::vec4& vec) const
	{
		int location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, vec.x, vec.y, vec.z, vec.w);
	}

	void Shader::SetInt(const std::string& name, int value) const
	{
		int location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

}