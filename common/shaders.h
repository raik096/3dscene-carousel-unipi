#ifndef MATERIAL_shad_H
#define MATERIAL_shad_H
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <fstream>
#include <regex>
#include "../common/debugging.h"

struct shader{
        GLuint   vertex_shader, geometry_shader, compute_shader, fragment_shader, program;

        std::map<std::string,int> uni;

        void bind(std::string name){
            uni[name] = glGetUniformLocation(program, name.c_str());
        }

		const bool has_uniform(std::string name) const  {
			return (uni.find(name) != uni.end());
		}
		
		int operator[](std::string name){

			//Se non abbiamo mai cachato l'id della uniform...
			if (uni.find(name) == uni.end()) {
				
				//Prova a cercarla nella shader
				int uniformLocation = glGetUniformLocation(program, name.c_str());
				if(uniformLocation != -1)
				{
					//Se la trovi fai il bind e ritorna
            		uni[name] = glGetUniformLocation(program, name.c_str());
					return uni[name];
				}

				//Se no dai errore
				std::cout << "No location for uniform variable " << name << std::endl;
				//exit(0);
			}

			//Se l'avevamo già cachata restituisci ebbasta
            return uni[name];
        }

		void SetInt(std::string uName, int value)
		{
			glUniform1i(uni[uName], value);
		}
		
		void SetBool(std::string uName, bool value)
		{
			SetInt(uName, (value)?1:0);
		}

		void SetFloat(std::string uName, float value)
		{
			glUniform1f(uni[uName], value);
		}

		void SetVector2(std::string uName, glm::vec2 value)
		{
			glUniform2f(uni[uName], value.x, value.y);
		}

		void SetVector3(std::string uName, glm::vec3 value)
		{
			glUniform3f(uni[uName], value.x, value.y, value.z);
		}

		void SetMatrix4x4(std::string uName, glm::mat4 value)
		{
			glUniformMatrix4fv(uni[uName], 1, GL_FALSE, &value[0][0]);
		}

#if defined(GL_VERSION_4_3)
		void  create_program(const GLchar* nameC) {
			std::string compute_shader_src_code = textFileRead(nameC);
			create_shader(compute_shader_src_code.c_str(), GL_COMPUTE_SHADER);
			program = glCreateProgram();
			glAttachShader(program, compute_shader);
			 
			glLinkProgram(program);

			bind_uniform_variables(compute_shader_src_code);
			check_shader(compute_shader);
			validate_shader_program(program);
		}
#endif
		/* create a program shader */
		void  create_program( const GLchar *nameV, const char *nameF){
		
			std::string vertex_shader_src_code  = textFileRead(nameV);
			std::string fragment_shader_src_code  = textFileRead(nameF);

			create_shader(vertex_shader_src_code.c_str(), GL_VERTEX_SHADER);
			create_shader(fragment_shader_src_code.c_str(), GL_FRAGMENT_SHADER);

			program = glCreateProgram();
			glAttachShader(program,vertex_shader);
			glAttachShader(program,fragment_shader);

			glLinkProgram(program);

			bind_uniform_variables(vertex_shader_src_code);
			bind_uniform_variables(fragment_shader_src_code);

			check_shader(vertex_shader);
			check_shader(fragment_shader);
			validate_shader_program(program);
		}

		void  create_program( const GLchar *nameG, const GLchar *nameV, const char *nameF){
		
			std::string geometry_shader_src_code  = textFileRead(nameG);
			std::string vertex_shader_src_code  = textFileRead(nameV);
			std::string fragment_shader_src_code  = textFileRead(nameF);

			create_shader(geometry_shader_src_code.c_str(), GL_GEOMETRY_SHADER);
			create_shader(vertex_shader_src_code.c_str(), GL_VERTEX_SHADER);
			create_shader(fragment_shader_src_code.c_str(), GL_FRAGMENT_SHADER);

			program = glCreateProgram();
			glAttachShader(program,geometry_shader);
			glAttachShader(program,vertex_shader);
			glAttachShader(program,fragment_shader);

			glLinkProgram(program);

			bind_uniform_variables(vertex_shader_src_code);
			bind_uniform_variables(fragment_shader_src_code);

			check_shader(geometry_shader);
			check_shader(vertex_shader);
			check_shader(fragment_shader);
			validate_shader_program(program);
		}

private:
		static  std::string textFileRead(const char* fn) {
			std::ifstream ifragment_shader(fn);
			std::string content((std::istreambuf_iterator<char>(ifragment_shader)),
				(std::istreambuf_iterator<char>()));
			if (content.empty()) {
				std::cout << "No content for " << fn << std::endl;
				exit(0);
			}
			return content;
		}

		bool create_shader(const GLchar* src, unsigned int SHADER_TYPE) {
			GLuint s = 0;
			switch (SHADER_TYPE) {
			case GL_VERTEX_SHADER:   s = vertex_shader = glCreateShader(GL_VERTEX_SHADER);break;
			case GL_FRAGMENT_SHADER: s = fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);break;
			case GL_GEOMETRY_SHADER: s = geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);break;
#if defined(GL_VERSION_4_3)
			case GL_COMPUTE_SHADER:  s = compute_shader = glCreateShader(GL_COMPUTE_SHADER);break;
#endif
			}

			glShaderSource(s, 1, &src, NULL);
			glCompileShader(s);
			int status;
			glGetShaderiv(s, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE) {
				check_shader(s);
				return false;
			}
			return true;
		}

		/* this function look for uniform variables and associates their locations
		 to their name in the map "uni" so that you can call
		 glUniform*(my_shader["uMyUniformName"],...)
		 Warning: this does NOT do a proper parsing of the code.
		 If your declare a list of uniforms with a single instance of the qualifier:

		 uniform float a,b[10],c;

		 it will only see c.
		 If you declare uniforms by themselves:

		 uniform float a;
		 uniform float b[10];
		 uniform float c;

		 it'll be fine
		 */
		void bind_uniform_variables(std::string code) {

                        code.erase(std::remove(code.begin(), code.end(), '\r'), code.end());
                        code.erase(std::remove(code.begin(), code.end(), '\n'), code.end());
			code.erase(std::remove(code.begin(), code.end(), '\t'), code.end());
			code.erase(std::remove(code.begin(), code.end(), '\b'), code.end());

			size_t pos;
			std::istringstream check1(code);

			std::string intermediate;
			std::vector <std::string> tokens;
			// Tokenizing w.r.t. space ' '
			while (getline(check1, intermediate, ';'))
			{
				intermediate = std::regex_replace(intermediate, std::regex("  "), " ");

				if (intermediate.find(" ") == 0)
					intermediate.erase(0, 1);

				if (intermediate.find("uniform") == 0) {
					pos = intermediate.find_last_of(" ");
					size_t pos_end = intermediate.find_first_of("[") - 1;
					if (pos_end > intermediate.length())
						pos_end = intermediate.length();
					std::string uniform_name = intermediate.substr(pos + 1, pos_end - pos);
					this->bind(uniform_name);
					tokens.push_back(intermediate.substr(pos + 1, intermediate.length() - pos));
				}
			}
		}

};

 #endif