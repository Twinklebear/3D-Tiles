#include <iostream>
#include <string>
#include <SDL.h>
#include <glm/glm.hpp>
#include "gl_core_4_4.h"
#include "util.h"

/*
 * The OpenGL DrawElementsIndirectCommand struct described in the docs
 */
struct DrawElementsIndirectCommand {
	GLuint count, instance_count, first_index,
		base_vertex, base_instance;

	DrawElementsIndirectCommand(GLuint count, GLuint instance_count, GLuint first_index,
		GLuint base_vertex, GLuint base_instance)
		: count(count), instance_count(instance_count), first_index(first_index),
		base_vertex(base_vertex), base_instance(base_instance)
	{}
};

//Our two "models" packed into a single buffer, really just two tris
const std::array<glm::vec3, 6> triangles_verts{
	//The first "model", lower left tri
	glm::vec3{-1.f, -1.f, 0.f}, glm::vec3{1.f, -1.f, 0.f}, glm::vec3{-1.f, 1.f, 0.f},
	//The second "model", upper right tri
	glm::vec3{1.f, -1.f, 0.f}, glm::vec3{1.f, 1.f, 0.f}, glm::vec3{-1.f, 1.f, 0.f}
};
//The element buffers for both "models" packed into one buffer
const std::array<GLushort, 6> triangles_indices{
	0, 1, 2,
	3, 4, 5
};
//Our attributes buffer for the two instances of our "models"
const std::array<glm::vec3, 2> triangle_attribs{
	glm::vec3{1.f, 0.f, 0.f}, glm::vec3{0.f, 0.f, 1.f}
};

int main(int, char**){
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	SDL_Window *win = SDL_CreateWindow("3D Tiles", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(1);

	if (ogl_LoadFunctions() == ogl_LOAD_FAILED){
		std::cerr << "ogl load failed\n";
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClearDepth(1.f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n"
		<< "OpenGL Vendor: " << glGetString(GL_VENDOR) << "\n"
		<< "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(util::gldebug_callback, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
#endif

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Vertex buffer, element buffer and instance attribute buffer
	GLuint vbo, ebo, attribs;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	glGenBuffers(1, &attribs);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangles_verts), triangles_verts.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles_indices), triangles_indices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, attribs);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_attribs), triangle_attribs.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(1, 1);

	const std::string shader_path = util::get_resource_path("shaders");
	GLuint shader = util::load_program({std::make_tuple(GL_VERTEX_SHADER, shader_path + "vmdei_test.glsl"),
		std::make_tuple(GL_FRAGMENT_SHADER, shader_path + "fmdei_test.glsl")});
	glUseProgram(shader);

	std::array<DrawElementsIndirectCommand, 2> draw_commands{
		DrawElementsIndirectCommand{3, 1, 0, 0, 0},
		DrawElementsIndirectCommand{3, 1, 3, 0, 1}
	};
	GLuint draw_cmd_buf;
	glGenBuffers(1, &draw_cmd_buf);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_cmd_buf);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(draw_commands), draw_commands.data(), GL_STATIC_DRAW);

	SDL_Event e;
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&e)){
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)){
				quit = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, 2, sizeof(DrawElementsIndirectCommand));

		SDL_GL_SwapWindow(win);
	}

	glDeleteProgram(shader);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &draw_cmd_buf);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &attribs);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}

