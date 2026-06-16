#pragma once

#include "../Core.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace Pondo {

	struct Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
	};

#pragma warning(push)
#pragma warning(disable: 4251)
	class PONDO_API Mesh {
	public:
		Mesh(const std::vector<Vertex>& vertices,
			const std::vector<unsigned int>& indices);
		~Mesh();

		void Bind()   const;
		void Unbind() const;

		unsigned int GetIndexCount()  const { return m_IndexCount; }
		unsigned int GetVertexCount() const { return m_VertexCount; }
		const std::string& GetType()  const { return m_Type; }
		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		const std::vector<unsigned int>& GetIndices()  const { return m_Indices; }

		static std::shared_ptr<Mesh> CreateCube();
		static std::shared_ptr<Mesh> CreatePlane(float size = 1.0f);
		static std::shared_ptr<Mesh> CreateSphere(int stacks = 16, int slices = 16);
		static std::shared_ptr<Mesh> CreateCylinder(int slices = 16);

	private:
		unsigned int m_VAO_ID = 0;
		unsigned int m_VBO_ID = 0;
		unsigned int m_IBO_ID = 0;
		unsigned int m_IndexCount = 0;
		unsigned int m_VertexCount = 0;
		std::string  m_Type = "Cube";

		std::vector<Vertex>       m_Vertices;
		std::vector<unsigned int> m_Indices;
	};
#pragma warning(pop)

}