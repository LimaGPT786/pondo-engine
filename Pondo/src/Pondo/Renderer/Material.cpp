#include "Material.h"

namespace Pondo {

	Material::Material(const std::shared_ptr<Shader>& shader)
		: m_Shader(shader)
	{
	}

	void Material::Bind() const
	{
		m_Shader->Bind();
		m_Shader->SetFloat4("u_Color", m_Color);
		m_Shader->SetFloat3("u_Color3", glm::vec3(m_Color));  // convenience alias
	}

}