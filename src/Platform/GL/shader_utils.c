/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */

#ifdef SDL2_RENDER

#include "shader_utils.h"
#include "Platform/GL/ShaderCompile.h"
#include "globals.h"

#include "SDL2_helper.h"
#include <stdio.h>
#include <string.h>

#include "stdPlatform.h"

#ifdef LINUX
#include "external/fcaseopen/fcaseopen.h"
#endif

#include "Platform/Common/stdEmbeddedRes.h"

/**
 * Display compilation errors from the OpenGL shader compiler
 */
void print_log(GLuint object) {
	GLint log_length = 0;
	if (glIsShader(object)) {
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else if (glIsProgram(object)) {
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR,
					   "printlog: Not a shader or a program");
		return;
	}

	char* log = (char*)malloc(log_length);
	
	if (glIsShader(object))
		glGetShaderInfoLog(object, log_length, NULL, log);
	else if (glIsProgram(object))
		glGetProgramInfoLog(object, log_length, NULL, log);
	
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "%s\n", log);
	
	free(log);
}

GLuint load_shader_file(const char* filepath, GLenum type)
{
    char* shader_contents = stdEmbeddedRes_Load(filepath, NULL);

    if (!shader_contents)
    {
        stdPlatform_Printf("std3D: Failed to load shader file `%s`!\n", filepath);
        return 0;
    }
    
    stdPlatform_Printf("std3D: Parse shader `%s`\n", filepath);
    
    ShaderStage stage = type == GL_VERTEX_SHADER ? SHADER_STAGE_VERTEX : SHADER_STAGE_FRAGMENT;
    ShaderCompileResult result = shader_compile_named(filepath, stage, shader_contents);
    free(shader_contents);
    return result.object;
}

/**
 * Compile the shader from file 'filename', with error handling
 */
GLuint create_shader(const char* shader, GLenum type) {
	ShaderStage stage = type == GL_VERTEX_SHADER ? SHADER_STAGE_VERTEX : SHADER_STAGE_FRAGMENT;
	return shader_compile_named("inline", stage, shader).object;
}
#endif // LINUX
