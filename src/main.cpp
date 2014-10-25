#include <iostream>
#include <string>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
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

	STD140Buffer<glm::mat4> viewing{2, GL_UNIFORM_BUFFER, GL_STATIC_DRAW};
	viewing.map(GL_WRITE_ONLY);
	viewing.write<0>(0) = glm::lookAt(glm::vec3{0.f, 0.f, 5.f}, glm::vec3{0.f, 0.f, 0.f},
		glm::vec3{0.f, 1.f, 0.f});
	viewing.write<0>(1) = glm::perspective(util::deg_to_rad(75.f), 640.f / 480.f, 1.f, 100.f);
	viewing.unmap();

	const std::string shader_path = util::get_resource_path("shaders");
	GLuint shader = util::load_program({std::make_tuple(GL_VERTEX_SHADER, shader_path + "vmdei_test.glsl"),
		std::make_tuple(GL_FRAGMENT_SHADER, shader_path + "fmdei_test.glsl")});
	glUseProgram(shader);
	GLuint viewing_block = glGetUniformBlockIndex(shader, "Viewing");
	glUniformBlockBinding(shader, viewing_block, 0);
	viewing.bind_base(0);

	const std::string model_path = util::get_resource_path("models");
	PackedBuffer<glm::vec3, glm::vec3, glm::vec3> vbo{0, GL_ARRAY_BUFFER, GL_STATIC_DRAW, true};
	PackedBuffer<GLushort> ebo{0, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, true};
	size_t tri_verts = 0, tri_elems = 0;
	if (!util::load_obj(model_path + "suzanne.obj", vbo, ebo, tri_elems, &tri_verts)){
		std::cout << "Failed to load left triangle\n";
		return 1;
	}
	size_t quad_verts = 0, quad_elems = 0;
	if (!util::load_obj(model_path + "polyhedron.obj", vbo, ebo, quad_elems, &quad_verts, tri_verts, tri_elems)){
		std::cout << "Failed to load quad\n";
		return 1;
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	vbo.bind();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vbo.stride(), 0);

	ebo.bind();

	PackedBuffer<glm::vec3, glm::mat4> attribs{2, GL_ARRAY_BUFFER, GL_STATIC_DRAW};
	attribs.map(GL_WRITE_ONLY);
	attribs.write<0>(0) = glm::vec3{1.f, 0.f, 0.f};
	attribs.write<1>(0) = glm::translate(glm::vec3{-2.f, 0.f, 0.f});
	attribs.write<0>(1) = glm::vec3{0.f, 0.f, 1.f};
	attribs.write<1>(1) = glm::translate(glm::vec3{2.f, 0.f, 0.f});
	attribs.unmap();
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, attribs.stride(), 0);
	glVertexAttribDivisor(1, 1);
	for (int i = 2; i < 6; ++i){
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, attribs.stride(),
			(void*)(attribs.offset(1) + (i - 2) * sizeof(glm::vec4)));
		glVertexAttribDivisor(i, 1);
	}

	std::array<DrawElementsIndirectCommand, 2> draw_commands{
		DrawElementsIndirectCommand{tri_elems, 1, 0, 0, 0},
		DrawElementsIndirectCommand{quad_elems, 1, tri_elems, 0, 1}
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

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}

