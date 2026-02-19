#include "Shader.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Shader.h"
#include "OpenGL.h"
#include <streambuf>
#include <algorithm>

#include "Simple.h"
#include "../Utils/Logging.h"

namespace Gorgon :: GL {
    GLuint activeprogram = -1;
    extern Gorgon::Utils::Logger log;


	GLuint CreateShader(GLenum type, const std::string& code, const std::string& name) {
		GLuint shader = glCreateShader(type);

		const GLchar* shader_code_str = code.data();
		glShaderSource(shader, 1, (const GLchar**)&shader_code_str, nullptr);
		glCompileShader(shader);

		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if(status == GL_FALSE) {
			GLint log_length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
			GLchar* shader_log_message = new GLchar[log_length];
			glGetShaderInfoLog(shader, log_length, nullptr, shader_log_message);
			log << name << " shader failed to compile: " << shader_log_message;
			delete[] shader_log_message;
		}
		log << "Compiled shader: " << name;
		return shader;
	}

	void InsertDefines(std::string& code, const std::string& shader_defines) {
        if(shader_defines=="") return;
        
		std::string::size_type pos = code.find("#version");
		if(pos==std::string::npos) {
			pos=0;
		}
		else {
            //find the end of the line
            while(code.length()>pos && code[pos]!='\n') pos++;
            
            //after that
            pos++;
        }
        
		int linesbefore=(int)std::count_if(code.begin(), code.begin()+pos, [](char c) { return c=='\n'; });

		const std::string insert_me = shader_defines + "#line " + std::to_string(linesbefore+1) + "\n";

		code.insert(pos, insert_me);
	}


	Shader::~Shader() {
		if(geometryshader) {
			glDetachShader(program, geometryshader);
			glDeleteShader(geometryshader);
		}

		if(fragmentshader) {
			glDetachShader(program, fragmentshader);
			glDeleteShader(fragmentshader);
		}

		if(vertexshader) {
            glDetachShader(program, vertexshader);
            glDeleteShader(vertexshader);
        }
        
        if(program) {   
            glDeleteProgram(program);
        }
	}

	void Shader::Use() {
        //if(activeprogram!=program) {
        //    activeprogram = program;
            glUseProgram(program);
        //}
	}

	unsigned int SetupUBO(int size, int binding_point) {
		GLuint ubo;
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, ubo);
		return ubo;
	}

	void UpdateUBO(unsigned int ubo, int size, void const * const data) {
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	}

	int Shader::LocateUniform(const std::string& name) {
		return glGetUniformLocation(program, name.data());
	}


	int Shader::BindTexture(const std::string& name, int location) {
		int ret = glGetUniformLocation(program, name.data());
		glUniform1i(ret, location);

		return ret;
	}

	void Shader::UpdateUniform(int name, float value) {
		glUniform1f(name, value);
#ifndef NDEBUG
		if(glGetError()!=0) {
			log << this->name << " update uniform " << name << " failed with value " << value;
		}
#endif
	}

	void Shader::UpdateUniform(int name, int value) {
		glUniform1i(name, value);
#ifndef NDEBUG
		if(glGetError()!=0) {
			log << this->name << " update uniform " << name << " failed with value " << value;
		}
#endif
	}

	void Shader::UpdateUniform(int name, const QuadVertices& value) {
		glUniform3fv(name, 4, (GLfloat*)value.Data);
#ifndef NDEBUG
		if(glGetError()!=0) {
			log << this->name << " update uniform " << name << " failed.";
		}
#endif
	}

	void Shader::UpdateUniform(int name, const QuadTextureCoords& value) {
		glUniform2fv(name, 4, (GLfloat*)value.Data);
#ifndef NDEBUG
		if(glGetError()!=0) {
			log << this->name << " update uniform " << name << " failed.";
		}
#endif
	}

	void Shader::UpdateUniform(int name, const Graphics::RGBAf& value) {
		glUniform4fv(name, 1, (GLfloat*)value.Vector);
#ifndef NDEBUG
		if(glGetError()!=0) {
			log << this->name << " update uniform " << name << " failed with value " << value;
		}
#endif
	}

	void Shader::UpdateUniform(int name, const Geometry::Point3D& value) {
		glUniform3fv(name, 1, (GLfloat*)value.Vector);
#ifndef NDEBUG
		if(glGetError()!=0) {
			log << this->name << " update uniform " << name << " failed with value " << value;
		}
#endif
	}

	void Shader::BindUBO(const std::string &name, UBOBindingPoint::Type binding_point) {
		auto index = glGetUniformBlockIndex(program, name.data());
		glUniformBlockBinding(program, index, binding_point);
#ifndef NDEBUG
		if(glGetError()!=0) {
			log << this->name << " update uniform " << name << " failed with value " << binding_point;
		}
#endif
	}

	void Shader::InitializeFromFiles(const std::string& vertexfile, const std::string& fragmentfile,
		const std::string& geometryfile, std::map<std::string, std::string> defines) {

		std::string vertexsrc, fragmentsrc, geometrysrc;

		std::ifstream file;
		
		file.open(vertexfile);
		if(!file.is_open()) {
			throw std::runtime_error("Cannot open vertex shader file");
		}

		vertexsrc={std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

		file.close();

		if(fragmentfile!="") {
			file.open(fragmentfile);
			if(!file.is_open()) {
				throw std::runtime_error("Cannot open fragment shader file");
			}

			fragmentsrc={std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

			file.close();
		}

		if(geometryfile!="") {
			file.open(fragmentfile);
			if(!file.is_open()) {
				throw std::runtime_error("Cannot open geometry shader file");
			}

			geometrysrc={std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

			file.close();
		}

		InitializeWithSource(vertexsrc, fragmentsrc, geometrysrc, defines);
	}

	void Shader::InitializeWithSource(std::string vertexsrc, std::string fragmentsrc,
		std::string geometrysrc, std::map<std::string, std::string> defines) {

		program = glCreateProgram();

		std::string definesstring;
		for(auto &p : defines) {
			definesstring+="#define "+p.first+"\t"+p.second+"\n";
		}
		
		InsertDefines(vertexsrc, definesstring);
        
        vertexshader = CreateShader(GL_VERTEX_SHADER, vertexsrc, "[vertex]"+name);
		glAttachShader(program, vertexshader);

		if(fragmentsrc!="") {
			InsertDefines(fragmentsrc, definesstring);

            fragmentshader = CreateShader(GL_FRAGMENT_SHADER, fragmentsrc, "[fragment]"+name);
			glAttachShader(program, fragmentshader);
		}
		
		if(geometrysrc!="") {
            InsertDefines(geometrysrc, definesstring);

            geometryshader = CreateShader(GL_GEOMETRY_SHADER, geometrysrc, "[geometry]"+name);
			glAttachShader(program, geometryshader);
		}

		glLinkProgram(program);

		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if(status == GL_FALSE) {
			GLint log_length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
			GLchar* program_log_message = new GLchar[log_length];
			glGetProgramInfoLog(program, log_length, nullptr, program_log_message);

			log << "Shader program "<< name <<" failed to link: " << std::endl << program_log_message;
			delete[] program_log_message;
		}
		log << "Shader program " << name << " successfully linked.\n";
	}

}
