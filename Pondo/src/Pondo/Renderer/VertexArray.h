#pragma once

#include "../Core.h"
#include "Buffer.h"
#include <memory>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)
	class PONDO_API VertexArray {
	public:
		VertexArray();
		~VertexArray();

		void Bind() const;
		void Unbind() const;

		void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
		void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

		const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }

	private:
		unsigned int m_RendererID;
		std::shared_ptr<VertexBuffer> m_VertexBuffer;
		std::shared_ptr<IndexBuffer>  m_IndexBuffer;
	};
#pragma warning(pop)

}