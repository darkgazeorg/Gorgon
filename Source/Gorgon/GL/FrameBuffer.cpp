#include "FrameBuffer.h"
#include "OpenGL.h"
#include "../Window.h"

namespace Gorgon :: GL {

	void FrameBuffer::Generate(bool gendepth) {
		if(!glBindFramebuffer) return;
		buffers.Add(this);

		auto sz = Window::GetMinimumRequiredSize();

		glGenFramebuffers(1, &buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, buffer);

		texture = GenerateEmptyTexture(sz, Graphics::ColorMode::RGBA);
        glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

		if(gendepth && glBindRenderbuffer) {
			glGenRenderbuffers(1, &depth);
			glBindRenderbuffer(GL_RENDERBUFFER, depth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, sz.Width, sz.Height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
		}
		
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		log << "Frame buffer texture name " << texture << ", depth name " << depth;
	}

	void FrameBuffer::UpdateSizes() {
		auto sz = Window::GetMinimumRequiredSize();

		for(FrameBuffer &buffer : buffers) {
			ResizeTexture(buffer.GetTexture(), sz, Graphics::ColorMode::RGBA);
			if(buffer.depth) {
				glBindRenderbuffer(GL_RENDERBUFFER, buffer.depth);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, sz.Width, sz.Height);
			}
		}
	}

	void FrameBuffer::Destroy() {
		if(!buffer) return;

		glDeleteFramebuffers(1, &buffer);
		buffer = 0;

		DestroyTexture(texture);
		texture = 0;

		if(depth) {
			glDeleteRenderbuffers(1, &depth);
			depth = 0;
		}
	}

	bool FrameBuffer::IsFunctional() const {
		return glBindFramebuffer!=nullptr;
	}

	void FrameBuffer::Use() {
		if(!buffer) return;

		glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	}

	void FrameBuffer::RenderToScreen() {
		GL::RenderToScreen();
	}

	bool FrameBuffer::HardwareSupport = false;
	Containers::Collection<FrameBuffer> FrameBuffer::buffers;
}
