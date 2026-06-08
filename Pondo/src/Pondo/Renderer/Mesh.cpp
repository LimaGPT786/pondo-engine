#include "Mesh.h"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace Pondo {

	Mesh::Mesh(const std::vector<Vertex>& vertices,
		const std::vector<unsigned int>& indices)
		: m_IndexCount((unsigned int)indices.size()),
		m_VertexCount((unsigned int)vertices.size())
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
		return std::make_shared<Mesh>(verts, idx);
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
		return std::make_shared<Mesh>(verts, idx);
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
		return std::make_shared<Mesh>(verts, idx);
	}

	std::shared_ptr<Mesh> Mesh::CreateCylinder(
		float radius,
		float height,
		int segments)
	{
		std::vector<Vertex> verts;
		std::vector<unsigned int> idx;

		float half = height * 0.5f;

		// side vertices
		for (int i = 0; i <= segments; i++)
		{
			float t =
				((float)i / (float)segments)
				* glm::two_pi<float>();

			float x = cos(t) * radius;
			float z = sin(t) * radius;

			glm::vec3 normal =
				glm::normalize(
					glm::vec3(x, 0, z)
				);

			verts.push_back({
				{x,-half,z},
				normal,
				{(float)i / segments,0}
				});

			verts.push_back({
				{x,half,z},
				normal,
				{(float)i / segments,1}
				});
		}

		// side indices
		for (unsigned int i = 0; i < (unsigned int)segments; i++)
		{
			unsigned int b = i * 2;

			idx.insert(
				idx.end(),
				{
					b,
					b + 1,
					b + 2,

					b + 1,
					b + 3,
					b + 2
				}
			);
		}

		// top center
		unsigned int topCenter =
			(unsigned int)verts.size();

		verts.push_back({
			{0,half,0},
			{0,1,0},
			{0.5f,0.5f}
			});

		// bottom center
		unsigned int bottomCenter =
			(unsigned int)verts.size();

		verts.push_back({
			{0,-half,0},
			{0,-1,0},
			{0.5f,0.5f}
			});

		unsigned int ringStart =
			(unsigned int)verts.size();

		for (int i = 0; i < segments; i++)
		{
			float t =
				((float)i / segments)
				* glm::two_pi<float>();

			float x =
				cos(t) * radius;

			float z =
				sin(t) * radius;

			verts.push_back({
				{x,half,z},
				{0,1,0},
				{0,0}
				});

			verts.push_back({
				{x,-half,z},
				{0,-1,0},
				{0,0}
				});
		}

		// caps
		for (unsigned int i = 0; i < (unsigned int)segments; i++)
		{
			unsigned int next =
				(i + 1)
				%
				(unsigned int)segments;

			idx.insert(
				idx.end(),
				{
					topCenter,
					ringStart + i * 2,
					ringStart + next * 2
				}
			);

			idx.insert(
				idx.end(),
				{
					bottomCenter,
					ringStart + next * 2 + 1,
					ringStart + i * 2 + 1
				}
			);
		}

		return std::make_shared<Mesh>(
			verts,
			idx
		);
	}
}