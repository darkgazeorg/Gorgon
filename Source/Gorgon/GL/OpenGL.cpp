#include "OpenGL.h"
#include "FrameBuffer.h"

#include "../GL.h"
#include "../Graphics/Color.h"
#include "../Containers/Image.h"
#include "../OS.h"


#ifdef LINUX
#	include <GL/glx.h>
#endif

#include "../Utils/Logging.h"



#	define GL_BGR	0x80E0
#	define GL_BGRA	0x80E1
#	define GL_DEBUG_OUTPUT 0x92E0



PFNGLDRAWBUFFERSPROC 					glDrawBuffers;
PFNGLGETATTRIBLOCATIONPROC				glGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC				glBindAttribLocation;
PFNGLATTACHSHADERPROC					glAttachShader;
PFNGLBINDBUFFERPROC						glBindBuffer;
PFNGLBINDBUFFERBASEPROC					glBindBufferBase;
PFNGLBINDVERTEXARRAYPROC				glBindVertexArray;
PFNGLBUFFERDATAPROC						glBufferData;
PFNGLBUFFERSUBDATAPROC					glBufferSubData;
PFNGLCOMPILESHADERPROC					glCompileShader;
PFNGLCREATEPROGRAMPROC					glCreateProgram;
PFNGLCREATESHADERPROC					glCreateShader;
PFNGLDELETEBUFFERSPROC					glDeleteBuffers;
PFNGLDELETEVERTEXARRAYSPROC				glDeleteVertexArrays;
PFNGLDELETEPROGRAMPROC					glDeleteProgram;
PFNGLDELETESHADERPROC					glDeleteShader;
PFNGLDETACHSHADERPROC					glDetachShader;
PFNGLENABLEVERTEXATTRIBARRAYPROC		glEnableVertexAttribArray;
PFNGLGENBUFFERSPROC						glGenBuffers;
PFNGLGENERATEMIPMAPPROC					glGenerateMipmap;
PFNGLGENVERTEXARRAYSPROC				glGenVertexArrays;
PFNGLGETPROGRAMINFOLOGPROC				glGetProgramInfoLog;
PFNGLGETPROGRAMIVPROC					glGetProgramiv;
PFNGLGETSHADERINFOLOGPROC				glGetShaderInfoLog;
PFNGLGETSHADERIVPROC					glGetShaderiv;
PFNGLGETUNIFORMBLOCKINDEXPROC			glGetUniformBlockIndex;
PFNGLGETUNIFORMLOCATIONPROC				glGetUniformLocation;
PFNGLLINKPROGRAMPROC					glLinkProgram;
PFNGLMAPBUFFERPROC						glMapBuffer;
PFNGLMAPBUFFERRANGEPROC					glMapBufferRange;
PFNGLSHADERSOURCEPROC					glShaderSource;
PFNGLUNIFORM1FPROC						glUniform1f;
PFNGLUNIFORM1IPROC						glUniform1i;
PFNGLUNIFORM2FVPROC						glUniform2fv;
PFNGLUNIFORM3FVPROC						glUniform3fv;
PFNGLUNIFORM4FVPROC						glUniform4fv;
PFNGLUNIFORMBLOCKBINDINGPROC			glUniformBlockBinding;
PFNGLUNIFORMMATRIX3FVPROC				glUniformMatrix3fv;
PFNGLUNIFORMMATRIX3X2FVPROC				glUniformMatrix3x2fv;
PFNGLUNIFORMMATRIX4FVPROC				glUniformMatrix4fv;
PFNGLUNIFORMMATRIX4X2FVPROC				glUniformMatrix4x2fv;
PFNGLUNIFORMMATRIX4X3FVPROC				glUniformMatrix4x3fv;
PFNGLUNMAPBUFFERPROC					glUnmapBuffer;
PFNGLUSEPROGRAMPROC						glUseProgram;
PFNGLVERTEXATTRIBPOINTERPROC			glVertexAttribPointer;
PFNGLVERTEXATTRIBIPOINTERPROC			glVertexAttribIPointer;
PFNGLBLENDFUNCSEPARATEPROC				glBlendFuncSeparate;
PFNGLDEBUGMESSAGECALLBACKPROC			glDebugMessageCallback;

PFNGLBINDFRAMEBUFFERPROC				glBindFramebuffer;
PFNGLCHECKFRAMEBUFFERSTATUSPROC			glCheckFramebufferStatus;
PFNGLDELETEFRAMEBUFFERSPROC				glDeleteFramebuffers;
PFNGLFRAMEBUFFERTEXTURE2DPROC			glFramebufferTexture2D;
PFNGLGENFRAMEBUFFERSPROC				glGenFramebuffers;
PFNGLBINDRENDERBUFFER					glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGE				glRenderbufferStorage;
PFNGLGENRENDERBUFFERS					glGenRenderbuffers;
PFNGLFRAMEBUFFERRENDERBUFFER			glFramebufferRenderbuffer;
PFNGLDELETERENDERBUFFERS				glDeleteRenderbuffers;


namespace Gorgon :: GL {
	Gorgon::Utils::Logger log;

	 void 
#ifdef WIN32
	 _stdcall 
#endif
	 debug_proc(GLenum source,
                GLenum type,
                GLuint id,
                GLenum severity,
                GLsizei length,
                const GLchar *message,
                const void *) {

		switch(id) {
		case 131218: // NVIDIA: "shader will be recompiled due to GL state mismatches"
			return;
		}

		log << message;
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

	void SetDefaultClear() {
		glClearColor(0.4f, 0.2f, 0.0f, 0.0f);
		glClearDepth(1.0f);
	}

	void Resize(const Geometry::Size &size) {
		glViewport(0, 0, size.Width, size.Height);					// Reset The Current Viewport
	}

	void Clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void LoadFunctions() {
#ifdef WIN32
		glDrawBuffers					= (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
		glGetAttribLocation				= (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
		glBindAttribLocation			= (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
		glActiveTexture					= (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
		glAttachShader					= (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		glBindBuffer					= (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
		glBindBufferBase				= (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
		glBindVertexArray				= (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
		glBufferData					= (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
		glBufferSubData					= (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
		glCompileShader					= (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		glCreateProgram					= (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		glCreateShader					= (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glDeleteBuffers					= (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
		glDeleteVertexArrays			= (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
		glDeleteProgram					= (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
		glDeleteShader					= (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
		glDetachShader					= (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
		glEnableVertexAttribArray		= (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
		glGenBuffers					= (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
		glGenerateMipmap				= (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
		glGenVertexArrays				= (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
		glGetProgramInfoLog				= (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
		glGetProgramiv					= (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		glGetShaderInfoLog				= (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		glGetShaderiv					= (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
		glGetUniformBlockIndex			= (PFNGLGETUNIFORMBLOCKINDEXPROC)wglGetProcAddress("glGetUniformBlockIndex");
		glGetUniformLocation			= (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		glLinkProgram					= (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		glMapBuffer						= (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
		glMapBufferRange				= (PFNGLMAPBUFFERRANGEPROC)wglGetProcAddress("glMapBufferRange");
		glShaderSource					= (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glUniform1f						= (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
		glUniform1i						= (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
		glUniform2fv					= (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv");
		glUniform3fv					= (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
		glUniform4fv					= (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
		glUniformBlockBinding			= (PFNGLUNIFORMBLOCKBINDINGPROC)wglGetProcAddress("glUniformBlockBinding");
		glUniformMatrix3fv				= (PFNGLUNIFORMMATRIX3FVPROC)wglGetProcAddress("glUniformMatrix3fv");
		glUniformMatrix3x2fv			= (PFNGLUNIFORMMATRIX3X2FVPROC)wglGetProcAddress("glUniformMatrix3x2fv");
		glUniformMatrix4fv				= (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
		glUniformMatrix4x2fv			= (PFNGLUNIFORMMATRIX4X2FVPROC)wglGetProcAddress("glUniformMatrix4x2fv");
		glUniformMatrix4x3fv			= (PFNGLUNIFORMMATRIX4X3FVPROC)wglGetProcAddress("glUniformMatrix4x3fv");
		glUnmapBuffer					= (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
		glUseProgram					= (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
		glVertexAttribPointer			= (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
		glVertexAttribIPointer			= (PFNGLVERTEXATTRIBIPOINTERPROC)wglGetProcAddress("glVertexAttribIPointer");
		glBlendFuncSeparate				= (PFNGLBLENDFUNCSEPARATEPROC)wglGetProcAddress("glBlendFuncSeparate");

		glDebugMessageCallback			= (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");

		glBindFramebuffer				= (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		glCheckFramebufferStatus		= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
		glDeleteFramebuffers			= (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
		glFramebufferTexture2D			= (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		glGenFramebuffers				= (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		if(!glBindFramebuffer) {
			glBindFramebuffer				= (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebufferExt");
			glCheckFramebufferStatus		= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatusExt");
			glDeleteFramebuffers			= (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffersExt");
			glFramebufferTexture2D			= (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2DExt");
			glGenFramebuffers				= (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffersExt");
		}
		if(glBindFramebuffer)
			FrameBuffer::HardwareSupport = true;

		glBindRenderbuffer				= (PFNGLBINDRENDERBUFFER)wglGetProcAddress("glBindRenderbuffer");
		glRenderbufferStorage			= (PFNGLRENDERBUFFERSTORAGE)wglGetProcAddress("glRenderbufferStorage");
		glGenRenderbuffers				= (PFNGLGENRENDERBUFFERS)wglGetProcAddress("glGenRenderbuffers");
		glFramebufferRenderbuffer		= (PFNGLFRAMEBUFFERRENDERBUFFER)wglGetProcAddress("glFramebufferRenderbuffer");
		glDeleteRenderbuffers			= (PFNGLDELETERENDERBUFFERS)wglGetProcAddress("glDeleteRenderbuffers");
		if(!glBindRenderbuffer) {
			glBindRenderbuffer				= (PFNGLBINDRENDERBUFFER)wglGetProcAddress("glBindRenderbufferExt");
			glRenderbufferStorage			= (PFNGLRENDERBUFFERSTORAGE)wglGetProcAddress("glRenderbufferStorageExt");
			glGenRenderbuffers				= (PFNGLGENRENDERBUFFERS)wglGetProcAddress("glGenRenderbuffersExt");
			glFramebufferRenderbuffer		= (PFNGLFRAMEBUFFERRENDERBUFFER)wglGetProcAddress("glFramebufferRenderbufferExt");
			glDeleteRenderbuffers			= (PFNGLDELETERENDERBUFFERS)wglGetProcAddress("glDeleteRenderbuffersExt");
		}

#elif defined(LINUX)
		//= ()wglGetProcAddress("gl");
		//glActiveTexture					= (PFNGLACTIVETEXTUREPROC)glXGetProcAddress((const GLubyte*)"glActiveTexture");
		glDrawBuffers					= (PFNGLDRAWBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDrawBuffers");
		glGetAttribLocation				= (PFNGLGETATTRIBLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glGetAttribLocation");
		glBindAttribLocation			= (PFNGLBINDATTRIBLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glBindAttribLocation");
		glAttachShader					= (PFNGLATTACHSHADERPROC)glXGetProcAddress((const GLubyte*)"glAttachShader");
		glBindBuffer					= (PFNGLBINDBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindBuffer");
		glBindBufferBase				= (PFNGLBINDBUFFERBASEPROC)glXGetProcAddress((const GLubyte*)"glBindBufferBase");
		glBindVertexArray				= (PFNGLBINDVERTEXARRAYPROC)glXGetProcAddress((const GLubyte*)"glBindVertexArray");
		glBufferData					= (PFNGLBUFFERDATAPROC)glXGetProcAddress((const GLubyte*)"glBufferData");
		glBufferSubData					= (PFNGLBUFFERSUBDATAPROC)glXGetProcAddress((const GLubyte*)"glBufferSubData");
		glCompileShader					= (PFNGLCOMPILESHADERPROC)glXGetProcAddress((const GLubyte*)"glCompileShader");
		glCreateProgram					= (PFNGLCREATEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glCreateProgram");
		glCreateShader					= (PFNGLCREATESHADERPROC)glXGetProcAddress((const GLubyte*)"glCreateShader");
		glDeleteBuffers					= (PFNGLDELETEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteBuffers");
		glDeleteVertexArrays			= (PFNGLDELETEVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte*)"glDeleteVertexArrays");
		glDeleteProgram					= (PFNGLDELETEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glDeleteProgram");
		glDeleteShader					= (PFNGLDELETESHADERPROC)glXGetProcAddress((const GLubyte*)"glDeleteShader");
		glDetachShader					= (PFNGLDETACHSHADERPROC)glXGetProcAddress((const GLubyte*)"glDetachShader");
		glEnableVertexAttribArray		= (PFNGLENABLEVERTEXATTRIBARRAYPROC)glXGetProcAddress((const GLubyte*)"glEnableVertexAttribArray");
		glGenBuffers					= (PFNGLGENBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenBuffers");
		glGenerateMipmap				= (PFNGLGENERATEMIPMAPPROC)glXGetProcAddress((const GLubyte*)"glGenerateMipmap");
		glGenVertexArrays				= (PFNGLGENVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte*)"glGenVertexArrays");
		glGetProgramInfoLog				= (PFNGLGETPROGRAMINFOLOGPROC)glXGetProcAddress((const GLubyte*)"glGetProgramInfoLog");
		glGetProgramiv					= (PFNGLGETPROGRAMIVPROC)glXGetProcAddress((const GLubyte*)"glGetProgramiv");
		glGetShaderInfoLog				= (PFNGLGETSHADERINFOLOGPROC)glXGetProcAddress((const GLubyte*)"glGetShaderInfoLog");
		glGetShaderiv					= (PFNGLGETSHADERIVPROC)glXGetProcAddress((const GLubyte*)"glGetShaderiv");
		glGetUniformBlockIndex			= (PFNGLGETUNIFORMBLOCKINDEXPROC)glXGetProcAddress((const GLubyte*)"glGetUniformBlockIndex");
		glGetUniformLocation			= (PFNGLGETUNIFORMLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glGetUniformLocation");
		glLinkProgram					= (PFNGLLINKPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glLinkProgram");
		glMapBuffer						= (PFNGLMAPBUFFERPROC)glXGetProcAddress((const GLubyte*)"glMapBuffer");
		glMapBufferRange				= (PFNGLMAPBUFFERRANGEPROC)glXGetProcAddress((const GLubyte*)"glMapBufferRange");
		glShaderSource					= (PFNGLSHADERSOURCEPROC)glXGetProcAddress((const GLubyte*)"glShaderSource");
		glUniform1f						= (PFNGLUNIFORM1FPROC)glXGetProcAddress((const GLubyte*)"glUniform1f");
		glUniform1i						= (PFNGLUNIFORM1IPROC)glXGetProcAddress((const GLubyte*)"glUniform1i");
		glUniform2fv					= (PFNGLUNIFORM2FVPROC)glXGetProcAddress((const GLubyte*)"glUniform2fv");
		glUniform3fv					= (PFNGLUNIFORM3FVPROC)glXGetProcAddress((const GLubyte*)"glUniform3fv");
		glUniform4fv					= (PFNGLUNIFORM4FVPROC)glXGetProcAddress((const GLubyte*)"glUniform4fv");
		glUniformBlockBinding			= (PFNGLUNIFORMBLOCKBINDINGPROC)glXGetProcAddress((const GLubyte*)"glUniformBlockBinding");
		glUniformMatrix3fv				= (PFNGLUNIFORMMATRIX3FVPROC)glXGetProcAddress((const GLubyte*)"glUniformMatrix3fv");
		glUniformMatrix3x2fv			= (PFNGLUNIFORMMATRIX3X2FVPROC)glXGetProcAddress((const GLubyte*)"glUniformMatrix3x2fv");
		glUniformMatrix4fv				= (PFNGLUNIFORMMATRIX4FVPROC)glXGetProcAddress((const GLubyte*)"glUniformMatrix4fv");
		glUniformMatrix4x2fv			= (PFNGLUNIFORMMATRIX4X2FVPROC)glXGetProcAddress((const GLubyte*)"glUniformMatrix4x2fv");
		glUniformMatrix4x3fv			= (PFNGLUNIFORMMATRIX4X3FVPROC)glXGetProcAddress((const GLubyte*)"glUniformMatrix4x3fv");
		glUnmapBuffer					= (PFNGLUNMAPBUFFERPROC)glXGetProcAddress((const GLubyte*)"glUnmapBuffer");
		glUseProgram					= (PFNGLUSEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glUseProgram");
		glVertexAttribPointer			= (PFNGLVERTEXATTRIBPOINTERPROC)glXGetProcAddress((const GLubyte*)"glVertexAttribPointer");
		glVertexAttribIPointer			= (PFNGLVERTEXATTRIBIPOINTERPROC)glXGetProcAddress((const GLubyte*)"glVertexAttribIPointer");
		glBlendFuncSeparate				= (PFNGLBLENDFUNCSEPARATEPROC)glXGetProcAddress((const GLubyte*)"glBlendFuncSeparate");

		glDebugMessageCallback			= (PFNGLDEBUGMESSAGECALLBACKPROC)glXGetProcAddress((const GLubyte*)"glDebugMessageCallback");

		glBindFramebuffer				= (PFNGLBINDFRAMEBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindFramebuffer");
		glCheckFramebufferStatus		= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glXGetProcAddress((const GLubyte*)"glCheckFramebufferStatus");
		glDeleteFramebuffers			= (PFNGLDELETEFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteFramebuffers");
		glFramebufferTexture2D			= (PFNGLFRAMEBUFFERTEXTURE2DPROC)glXGetProcAddress((const GLubyte*)"glFramebufferTexture2D");
		glGenFramebuffers				= (PFNGLGENFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenFramebuffers");
		if(!glBindFramebuffer) {
			glBindFramebuffer				= (PFNGLBINDFRAMEBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindFramebufferExt");
			glCheckFramebufferStatus		= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glXGetProcAddress((const GLubyte*)"glCheckFramebufferStatusExt");
			glDeleteFramebuffers			= (PFNGLDELETEFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteFramebuffersExt");
			glFramebufferTexture2D			= (PFNGLFRAMEBUFFERTEXTURE2DPROC)glXGetProcAddress((const GLubyte*)"glFramebufferTexture2DExt");
			glGenFramebuffers				= (PFNGLGENFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenFramebuffersExt");
		}
		if(glBindFramebuffer)
			FrameBuffer::HardwareSupport = true;


		glBindRenderbuffer				= (PFNGLBINDRENDERBUFFER)glXGetProcAddress((const GLubyte*)"glBindRenderbuffer");
		glRenderbufferStorage			= (PFNGLRENDERBUFFERSTORAGE)glXGetProcAddress((const GLubyte*)"glRenderbufferStorage");
		glGenRenderbuffers				= (PFNGLGENRENDERBUFFERS)glXGetProcAddress((const GLubyte*)"glGenRenderbuffers");
		glFramebufferRenderbuffer		= (PFNGLFRAMEBUFFERRENDERBUFFER)glXGetProcAddress((const GLubyte*)"glFramebufferRenderbuffer");
		glDeleteRenderbuffers			= (PFNGLDELETERENDERBUFFERS)glXGetProcAddress((const GLubyte*)"glDeleteRenderbuffers");
		if(!glBindRenderbuffer) {
			glBindRenderbuffer				= (PFNGLBINDRENDERBUFFER)glXGetProcAddress((const GLubyte*)"glBindRenderbufferExt");
			glRenderbufferStorage			= (PFNGLRENDERBUFFERSTORAGE)glXGetProcAddress((const GLubyte*)"glRenderbufferStorageExt");
			glGenRenderbuffers				= (PFNGLGENRENDERBUFFERS)glXGetProcAddress((const GLubyte*)"glGenRenderbuffersExt");
			glFramebufferRenderbuffer		= (PFNGLFRAMEBUFFERRENDERBUFFER)glXGetProcAddress((const GLubyte*)"glFramebufferRenderbufferExt");
			glDeleteRenderbuffers			= (PFNGLDELETERENDERBUFFERS)glXGetProcAddress((const GLubyte*)"glDeleteRenderbuffersExt");
		}

#endif
	}
	
}
