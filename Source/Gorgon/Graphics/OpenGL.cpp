
#include "OpenGL.h"

#ifdef WIN32
#	include <windows.h>
#elif defined(LINUX)
#	include <GL/glx.h>
#	include <unistd.h>
#endif

#ifndef LINUX
PFNGLACTIVETEXTUREPROC					glActiveTexture;
#endif
PFNGLDRAWBUFFERSPROC 					glDrawBuffers;
PFNGLGETATTRIBLOCATIONPROC				glGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC				glBindAttribLocation;
PFNGLATTACHSHADERPROC					glAttachShader;
PFNGLBINDBUFFERPROC						glBindBuffer;
PFNGLBINDBUFFERBASEPROC					glBindBufferBase;
PFNGLBINDFRAMEBUFFERPROC				glBindFramebuffer;
PFNGLBINDVERTEXARRAYPROC				glBindVertexArray;
PFNGLBUFFERDATAPROC						glBufferData;
PFNGLBUFFERSUBDATAPROC					glBufferSubData;
PFNGLCHECKFRAMEBUFFERSTATUSPROC			glCheckFramebufferStatus;
PFNGLCOMPILESHADERPROC					glCompileShader;
PFNGLCREATEPROGRAMPROC					glCreateProgram;
PFNGLCREATESHADERPROC					glCreateShader;
PFNGLDELETEBUFFERSPROC					glDeleteBuffers;
PFNGLDELETEFRAMEBUFFERSPROC				glDeleteFramebuffers;
PFNGLDELETEVERTEXARRAYSPROC				glDeleteVertexArrays;
PFNGLDELETEPROGRAMPROC					glDeleteProgram;
PFNGLDELETESHADERPROC					glDeleteShader;
PFNGLDETACHSHADERPROC					glDetachShader;
PFNGLENABLEVERTEXATTRIBARRAYPROC		glEnableVertexAttribArray;
PFNGLFRAMEBUFFERTEXTURE2DPROC			glFramebufferTexture2D;
PFNGLGENBUFFERSPROC						glGenBuffers;
PFNGLGENERATEMIPMAPPROC					glGenerateMipmap;
PFNGLGENFRAMEBUFFERSPROC				glGenFramebuffers;
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

namespace Gorgon :: GL {

	void LoadGLFunctions() {
#ifdef WIN32
			glDrawBuffers					= (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
			glGetAttribLocation				= (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
			glBindAttribLocation			= (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
			glActiveTexture					= (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
			glAttachShader					= (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
			glBindBuffer					= (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
			glBindBufferBase				= (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
			glBindFramebuffer				= (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
			glBindVertexArray				= (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
			glBufferData					= (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
			glBufferSubData					= (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
			glCheckFramebufferStatus		= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
			glCompileShader					= (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
			glCreateProgram					= (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
			glCreateShader					= (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
			glDeleteBuffers					= (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
			glDeleteFramebuffers			= (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
			glDeleteVertexArrays			= (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
			glDeleteProgram					= (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
			glDeleteShader					= (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
			glDetachShader					= (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
			glEnableVertexAttribArray		= (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
			glFramebufferTexture2D			= (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
			glGenBuffers					= (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
			glGenerateMipmap				= (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
			glGenFramebuffers				= (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
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
#elif defined(LINUX)
			//= ()wglGetProcAddress("gl");
			//glActiveTexture					= (PFNGLACTIVETEXTUREPROC)glXGetProcAddress((const GLubyte*)"glActiveTexture");
			glDrawBuffers					= (PFNGLDRAWBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDrawBuffers");
			glGetAttribLocation				= (PFNGLGETATTRIBLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glGetAttribLocation");
			glBindAttribLocation			= (PFNGLBINDATTRIBLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glBindAttribLocation");
			glAttachShader					= (PFNGLATTACHSHADERPROC)glXGetProcAddress((const GLubyte*)"glAttachShader");
			glBindBuffer					= (PFNGLBINDBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindBuffer");
			glBindBufferBase				= (PFNGLBINDBUFFERBASEPROC)glXGetProcAddress((const GLubyte*)"glBindBufferBase");
			glBindFramebuffer				= (PFNGLBINDFRAMEBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindFramebuffer");
			glBindVertexArray				= (PFNGLBINDVERTEXARRAYPROC)glXGetProcAddress((const GLubyte*)"glBindVertexArray");
			glBufferData					= (PFNGLBUFFERDATAPROC)glXGetProcAddress((const GLubyte*)"glBufferData");
			glBufferSubData					= (PFNGLBUFFERSUBDATAPROC)glXGetProcAddress((const GLubyte*)"glBufferSubData");
			glCheckFramebufferStatus		= (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glXGetProcAddress((const GLubyte*)"glCheckFramebufferStatus");
			glCompileShader					= (PFNGLCOMPILESHADERPROC)glXGetProcAddress((const GLubyte*)"glCompileShader");
			glCreateProgram					= (PFNGLCREATEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glCreateProgram");
			glCreateShader					= (PFNGLCREATESHADERPROC)glXGetProcAddress((const GLubyte*)"glCreateShader");
			glDeleteBuffers					= (PFNGLDELETEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteBuffers");
			glDeleteFramebuffers			= (PFNGLDELETEFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteFramebuffers");
			glDeleteVertexArrays			= (PFNGLDELETEVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte*)"glDeleteVertexArrays");
			glDeleteProgram					= (PFNGLDELETEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glDeleteProgram");
			glDeleteShader					= (PFNGLDELETESHADERPROC)glXGetProcAddress((const GLubyte*)"glDeleteShader");
			glDetachShader					= (PFNGLDETACHSHADERPROC)glXGetProcAddress((const GLubyte*)"glDetachShader");
			glEnableVertexAttribArray		= (PFNGLENABLEVERTEXATTRIBARRAYPROC)glXGetProcAddress((const GLubyte*)"glEnableVertexAttribArray");
			glFramebufferTexture2D			= (PFNGLFRAMEBUFFERTEXTURE2DPROC)glXGetProcAddress((const GLubyte*)"glFramebufferTexture2D");
			glGenBuffers					= (PFNGLGENBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenBuffers");
			glGenerateMipmap				= (PFNGLGENERATEMIPMAPPROC)glXGetProcAddress((const GLubyte*)"glGenerateMipmap");
			glGenFramebuffers				= (PFNGLGENFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenFramebuffers");
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
#endif
		}
}