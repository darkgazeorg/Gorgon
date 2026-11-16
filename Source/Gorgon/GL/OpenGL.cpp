#include "OpenGL.h"
#include "FrameBuffer.h"

#include "../GL.h"
#include "../Graphics/Color.h"
#include "../Containers/Image.h"
#include "../OS.h"
#include "../WindowManager.h"


#ifdef LINUX
#	include <GL/glx.h>
#elif defined(WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

#include "../Utils/Logging.h"



#	define GL_BGR	0x80E0
#	define GL_BGRA	0x80E1
#	define GL_DEBUG_OUTPUT 0x92E0



namespace Gorgon :: GL {
	Gorgon::Utils::Logger log;

	void GLAPIENTRY debug_proc(GLenum source, GLenum type, GLuint id, 
                               GLenum severity, GLsizei length, 
                               const GLchar* message, const void* userParam) {
        if (id == 131218) return; // Ignore NVIDIA recompilation warnings
        log << message;
    }

    void Initialize() {
        // Initialize GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        
        if (err != GLEW_OK) {
            log << "GLEW Initialization Error: " << (const char*)glewGetErrorString(err);
            return;
        }

        // Check for Framebuffer support (Available in Core 3.0+ or via extension)
        if (GLEW_VERSION_3_0 || GLEW_ARB_framebuffer_object) {
            FrameBuffer::HardwareSupport = true;
        }
        
        log << "OpenGL Functions Loaded via GLEW";
    }

	GLenum getGLColorMode(Graphics::ColorMode mode) {
		switch(mode) {
		case Graphics::ColorMode::Alpha:
			return GL_ALPHA;
		case Graphics::ColorMode::Grayscale_Alpha:
			return GL_LUMINANCE_ALPHA;
		case Graphics::ColorMode::Grayscale:
			return GL_LUMINANCE;
		case Graphics::ColorMode::BGR:
			return GL_BGR;
		case Graphics::ColorMode::RGB:
			return GL_RGB;
		case Graphics::ColorMode::BGRA:
			return GL_BGRA;
		case Graphics::ColorMode::RGBA:
			return GL_RGBA;
		default:
			return GL_RGBA;
		}
	}

	void settexturedata(Texture tex, const Containers::Image &data) {

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);

		glBindTexture(GL_TEXTURE_2D, tex);

		GLenum colormode=getGLColorMode(data.GetMode());

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		glTexImage2D(GL_TEXTURE_2D, 0, colormode, data.GetSize().Width, data.GetSize().Height, 0,
			colormode, GL_UNSIGNED_BYTE, data.RawData());
	}

	Texture GenerateTexture(const Containers::Image &data) {
		Texture tex;
		glGenTextures(1, &tex);
		settexturedata(tex, data);

		return tex;
	}

	Texture GenerateEmptyTexture(const Geometry::Size &size, Graphics::ColorMode mode) {
		Texture tex;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		GLenum colormode=getGLColorMode(mode);

		glTexImage2D(GL_TEXTURE_2D, 0, colormode, size.Width, size.Height, 0, colormode, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		return tex;
	}

	void ResizeTexture(Texture tex, const Geometry::Size &size, Graphics::ColorMode mode) {

		GLenum colormode=getGLColorMode(mode);

		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, colormode, size.Width, size.Height, 0, colormode, GL_UNSIGNED_BYTE, NULL);
	}

	void UpdateTexture(Texture tex, const Containers::Image &data) {
		settexturedata(tex, data);
	}

	void CopyToTexture(Texture tex, const Containers::Image &data, Geometry::Point target) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glBindTexture(GL_TEXTURE_2D, tex);

		GLenum colormode=getGLColorMode(data.GetMode());

		glTexSubImage2D(GL_TEXTURE_2D, 0, 
                        target.X, target.Y, data.GetWidth(), data.GetHeight(), 
                        colormode, GL_UNSIGNED_BYTE, data.RawData());
	}

	void CopyToTexture(Texture tex, const Containers::Image &data, Geometry::Bounds source, Geometry::Point target) {
		glPixelStorei(GL_UNPACK_ALIGNMENT,   1);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, source.Left);
		glPixelStorei(GL_UNPACK_SKIP_ROWS,   source.Top);
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  data.GetWidth());
        

		glBindTexture(GL_TEXTURE_2D, tex);

		GLenum colormode=getGLColorMode(data.GetMode());

		glTexSubImage2D(GL_TEXTURE_2D, 0, 
                        target.X, target.Y, source.Width(), source.Height(), 
                        colormode, GL_UNSIGNED_BYTE, data.RawData());
        
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS,   0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	}

	void DestroyTexture(Texture tex) {
		glDeleteTextures(1, &tex);
	}

	void ActivateTexture(Texture tex, int unit) {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, tex);
	}

	void RenderToTexture(FrameBuffer &buffer) {
		buffer.Use();
	}

	void RenderToScreen() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void SetupContext(const Geometry::Size &size) {
		std::string gl_version(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
		if(std::round(String::To<float>(gl_version)*10)<21) {
			OS::DisplayMessage("OpenGL version 2.1 and above is required. Your OpenGL version is "+gl_version);
			exit(2);
		}

		glShadeModel(GL_SMOOTH);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_BLEND);

		SetDefaultClear();
		SetDefaultBlending();

		glEnable(GL_TEXTURE_2D);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		glFrontFace(GL_CCW);

#ifndef NDEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(&debug_proc, nullptr);
#endif

		Resize(size);
		Clear();
		glFlush();
		glFinish();
	}
	
	void SetDefaultBlending() {
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
	}

	void SetMaskBlending() {
		glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ONE, GL_ZERO);
	}

	void SetDefaultClear() {
		glClearColor(0.4f, 0.2f, 0.0f, 0.0f);
		glClearDepth(1.0f);
	}

	void SetClearColor(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
	}

	bool IsClipEnabled() {
		return glIsEnabled(GL_SCISSOR_TEST);
	}

	void SetClipEnabled(bool enabled) {
		if(enabled)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}

	void SetClipRectangle(const Geometry::Rectangle &rectangle) {
		glScissor(rectangle.X, rectangle.Y, rectangle.Width, rectangle.Height);
	}

	namespace {
		int quadvertexindex[] ={
			0, 3, 1,
			1, 3, 2
		};

		GLuint quadvbo = 0;
		std::map<decltype(WindowManager::CurrentContext()), GLuint> vaos;
	}

	void SetupQuadVertices() {
		if(!quadvbo) {
			glGenBuffers(1, &quadvbo);

			glBindBuffer(GL_ARRAY_BUFFER, quadvbo);
			glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(int), quadvertexindex, GL_STATIC_DRAW);
		}

		if(vaos.count(WindowManager::CurrentContext())==0) {
			auto ctx=WindowManager::CurrentContext();
			glGenVertexArrays(1, &vaos[ctx]);
			glBindVertexArray(vaos[ctx]);

			glBindBuffer(GL_ARRAY_BUFFER, quadvbo);
			glEnableVertexAttribArray(0);
			glVertexAttribIPointer(0, 1, GL_INT, 0, (GLvoid*)0);
		}
	}

	void ActivateQuadVertices() {
#ifndef NDEBUG
		if(vaos.count(WindowManager::CurrentContext())==0) {
			throw std::logic_error("Context not initialized???");
		}
#endif
		glBindVertexArray(vaos[WindowManager::CurrentContext()]);
	}

	void DrawQuadVertices() {
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void Resize(const Geometry::Size &size) {
		glViewport(0, 0, size.Width, size.Height);					// Reset The Current Viewport
	}

	void Clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

}
