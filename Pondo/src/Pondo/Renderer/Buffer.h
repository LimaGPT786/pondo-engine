#pragma once

#include "../Core.h"

namespace Pondo {

	class PONDO_API VertexBuffer {
	public:
		VertexBuffer(float* vertices, unsigned int size);
		~VertexBuffer();

		void Bind() const;
		void Unbind() const;

	private:
		unsigned int m_RendererID;
	};

	class PONDO_API IndexBuffer {
	public:
		IndexBuffer(unsigned int* indices, unsigned int count);
		~IndexBuffer();

		void Bind() const;
		void Unbind() const;

		unsigned int GetCount() const { return m_Count; }

	private:
		unsigned int m_RendererID;
		unsigned int m_Count;
	};

}