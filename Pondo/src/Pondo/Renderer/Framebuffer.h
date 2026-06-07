#pragma once

#include "../Core.h"

namespace Pondo {

	struct FramebufferSpec {
		unsigned int Width = 1280;
		unsigned int Height = 720;
	};

	class PONDO_API Framebuffer {
	public:
		Framebuffer(const FramebufferSpec& spec);
		~Framebuffer();

		// Call this when the viewport panel is resized
		void Resize(unsigned int width, unsigned int height);

		void Bind()   const;
		void Unbind() const;

		// Returns the OpenGL texture ID — pass straight to ImGui::Image
		unsigned int GetColorAttachmentID() const { return m_ColorAttachment; }

		const FramebufferSpec& GetSpec() const { return m_Spec; }

	private:
		void Invalidate();

		FramebufferSpec m_Spec;
		unsigned int    m_RendererID = 0;
		unsigned int    m_ColorAttachment = 0;
		unsigned int    m_DepthAttachment = 0;
	};

}