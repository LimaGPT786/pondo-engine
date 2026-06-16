#include "Mesh.h"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace Pondo {

	Mesh::Mesh(const std::vector<Vertex>& vertices,
		const std::vector<unsigned int>& indices)
		: m_IndexCount((unsigned int)indices.size()),
		m_VertexCount((unsigned int)vertices.size()),
		m_Vertices(vertices),
		m_Indices(indices)
	{
		glGenVertexArrays(1, &m_VAO_ID);
		glBindVertexArray(m_VAO_ID);

		glGenBuffers(1, &m_VBO_ID);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID);
		glBufferData(GL_ARRAY_BUFFER,
			vertices.size() * sizeof(Vertex),
			vertices.data(), GL_STATIC_DRAW);

		// location 0: Position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), (void*)offsetof(Vertex, Position));
		// location 1: Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		// location 2: TexCoord
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));

		glGenBuffers(1, &m_IBO_ID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO_ID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			indices.size() * sizeof(unsigned int),
			indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

	Mesh::~Mesh()
	{
		glDeleteVertexArrays(1, &m_VAO_ID);
		glDeleteBuffers(1, &m_VBO_ID);
		glDeleteBuffers(1, &m_IBO_ID);
	}

	void Mesh::Bind()   const { glBindVertexArray(m_VAO_ID); }
	void Mesh::Unbind() const { glBindVertexArray(0); }

	std::shared_ptr<Mesh> Mesh::CreateCube()
	{
		std::vector<Vertex> verts = {
			// Front  (normal  0, 0, 1)
			{{ -0.5f, -0.5f,  0.5f }, { 0,0,1 }, { 0,0 }},
			{{  0.5f, -0.5f,  0.5f }, { 0,0,1 }, { 1,0 }},
			{{  0.5f,  0.5f,  0.5f }, { 0,0,1 }, { 1,1 }},
			{{ -0.5f,  0.5f,  0.5f }, { 0,0,1 }, { 0,1 }},
			// Back   (normal  0, 0,-1)
			{{  0.5f, -0.5f, -0.5f }, { 0,0,-1 }, { 0,0 }},
			{{ -0.5f, -0.5f, -0.5f }, { 0,0,-1 }, { 1,0 }},
			{{ -0.5f,  0.5f, -0.5f }, { 0,0,-1 }, { 1,1 }},
			{{  0.5f,  0.5f, -0.5f }, { 0,0,-1 }, { 0,1 }},
			// Left   (normal -1, 0, 0)
			{{ -0.5f, -0.5f, -0.5f }, {-1,0,0 }, { 0,0 }},
			{{ -0.5f, -0.5f,  0.5f }, {-1,0,0 }, { 1,0 }},
			{{ -0.5f,  0.5f,  0.5f }, {-1,0,0 }, { 1,1 }},
			{{ -0.5f,  0.5f, -0.5f }, {-1,0,0 }, { 0,1 }},
			// Right  (normal  1, 0, 0)
			{{  0.5f, -0.5f,  0.5f }, { 1,0,0 }, { 0,0 }},
			{{  0.5f, -0.5f, -0.5f }, { 1,0,0 }, { 1,0 }},
			{{  0.5f,  0.5f, -0.5f }, { 1,0,0 }, { 1,1 }},
			{{  0.5f,  0.5f,  0.5f }, { 1,0,0 }, { 0,1 }},
			// Top    (normal  0, 1, 0)
			{{ -0.5f,  0.5f,  0.5f }, { 0,1,0 }, { 0,0 }},
			{{  0.5f,  0.5f,  0.5f }, { 0,1,0 }, { 1,0 }},
			{{  0.5f,  0.5f, -0.5f }, { 0,1,0 }, { 1,1 }},
			{{ -0.5f,  0.5f, -0.5f }, { 0,1,0 }, { 0,1 }},
			// Bottom (normal  0,-1, 0)
			{{ -0.5f, -0.5f, -0.5f }, { 0,-1,0 }, { 0,0 }},
			{{  0.5f, -0.5f, -0.5f }, { 0,-1,0 }, { 1,0 }},
			{{  0.5f, -0.5f,  0.5f }, { 0,-1,0 }, { 1,1 }},
			{{ -0.5f, -0.5f,  0.5f }, { 0,-1,0 }, { 0,1 }},
		};

		std::vector<unsigned int> idx;
		for (unsigned int f = 0; f < 6; f++) {
			unsigned int b = f * 4;
			idx.insert(idx.end(), { b,b + 1,b + 2, b,b + 2,b + 3 });
		}

		auto m = std::make_shared<Mesh>(verts, idx);
		m->m_Type = "Cube";
		return m;
	}

	std::shared_ptr<Mesh> Mesh::CreatePlane(float size)
	{
		float h = size * 0.5f;
		std::vector<Vertex> verts = {
			{{ -h, 0.0f, -h }, { 0,1,0 }, { 0,0 }},
			{{  h, 0.0f, -h }, { 0,1,0 }, { 1,0 }},
			{{  h, 0.0f,  h }, { 0,1,0 }, { 1,1 }},
			{{ -h, 0.0f,  h }, { 0,1,0 }, { 0,1 }},
		};
		std::vector<unsigned int> idx = { 0,1,2, 0,2,3 };

		auto m = std::make_shared<Mesh>(verts, idx);
		m->m_Type = "Plane";
		return m;
	}

	std::shared_ptr<Mesh> Mesh::CreateSphere(int stacks, int slices)
	{
		std::vector<Vertex> verts;
		std::vector<unsigned int> idx;

		for (int i = 0; i <= stacks; i++) {
			float phi = glm::pi<float>() * ((float)i / stacks);
			for (int j = 0; j <= slices; j++) {
				float theta = 2.0f * glm::pi<float>() * ((float)j / slices);
				glm::vec3 pos = {
					std::sin(phi) * std::cos(theta),
					std::cos(phi),
					std::sin(phi) * std::sin(theta)
				};
				verts.push_back({
					pos * 0.5f,
					glm::normalize(pos),
					{ (float)j / slices, (float)i / stacks }
					});
			}
		}

		for (int i = 0; i < stacks; i++) {
			for (int j = 0; j < slices; j++) {
				unsigned int a = i * (slices + 1) + j;
				unsigned int b = a + slices + 1;
				idx.insert(idx.end(), { a, b, a + 1, b, b + 1, a + 1 });
			}
		}

		auto m = std::make_shared<Mesh>(verts, idx);
		m->m_Type = "Sphere";
		return m;
	}

	std::shared_ptr<Mesh> Mesh::CreateCylinder(int slices)
	{
		std::vector<Vertex> verts;
		std::vector<unsigned int> idx;

		const float radius = 0.5f;
		const float halfH = 0.5f;

		// Side vertices — two rings (bottom + top)
		for (int i = 0; i <= slices; i++) {
			float theta = 2.0f * glm::pi<float>() * ((float)i / slices);
			float c = std::cos(theta);
			float s = std::sin(theta);
			glm::vec3 normal = { c, 0.0f, s };

			// bottom ring
			verts.push_back({ { c * radius, -halfH, s * radius }, normal, { (float)i / slices, 0.0f } });
			// top ring
			verts.push_back({ { c * radius,  halfH, s * radius }, normal, { (float)i / slices, 1.0f } });
		}

		// Side indices
		for (int i = 0; i < slices; i++) {
			unsigned int b = i * 2;
			idx.insert(idx.end(), { b, b + 2, b + 1,  b + 1, b + 2, b + 3 });
		}

		// Cap centres
		unsigned int bottomCentre = (unsigned int)verts.size();
		verts.push_back({ { 0.0f, -halfH, 0.0f }, { 0,-1,0 }, { 0.5f, 0.5f } });
		unsigned int topCentre = (unsigned int)verts.size();
		verts.push_back({ { 0.0f,  halfH, 0.0f }, { 0, 1,0 }, { 0.5f, 0.5f } });

		// Cap rim vertices + indices
		for (int i = 0; i < slices; i++) {
			float t0 = 2.0f * glm::pi<float>() * ((float)i / slices);
			float t1 = 2.0f * glm::pi<float>() * ((float)(i + 1) / slices);

			// bottom cap
			unsigned int b0 = (unsigned int)verts.size();
			verts.push_back({ { std::cos(t0) * radius, -halfH, std::sin(t0) * radius }, { 0,-1,0 }, { 0.5f + std::cos(t0) * 0.5f, 0.5f + std::sin(t0) * 0.5f } });
			unsigned int b1 = (unsigned int)verts.size();
			verts.push_back({ { std::cos(t1) * radius, -halfH, std::sin(t1) * radius }, { 0,-1,0 }, { 0.5f + std::cos(t1) * 0.5f, 0.5f + std::sin(t1) * 0.5f } });
			idx.insert(idx.end(), { bottomCentre, b1, b0 });

			// top cap
			unsigned int t0i = (unsigned int)verts.size();
			verts.push_back({ { std::cos(t0) * radius,  halfH, std::sin(t0) * radius }, { 0, 1,0 }, { 0.5f + std::cos(t0) * 0.5f, 0.5f + std::sin(t0) * 0.5f } });
			unsigned int t1i = (unsigned int)verts.size();
			verts.push_back({ { std::cos(t1) * radius,  halfH, std::sin(t1) * radius }, { 0, 1,0 }, { 0.5f + std::cos(t1) * 0.5f, 0.5f + std::sin(t1) * 0.5f } });
			idx.insert(idx.end(), { topCentre, t0i, t1i });
		}

		auto m = std::make_shared<Mesh>(verts, idx);
		m->m_Type = "Cylinder";
		return m;
	}

}