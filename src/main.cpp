#include <iostream>
#include <string>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "gl_core_4_4.h"
#include "util.h"
#include "multi_renderbatch.h"

int main(int, char**){
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
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
	viewing.write<0>(0) = glm::lookAt(glm::vec3{0.f, 4.f, 5.f}, glm::vec3{0.f, 0.f, 0.f},
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
	size_t ma_verts = 0, ma_elems = 0;
	if (!util::load_obj(model_path + "dented_tile.obj", vbo, ebo, ma_elems, &ma_verts)){
		std::cout << "Failed to load left triangle\n";
		return 1;
	}
	size_t mb_verts = 0, mb_elems = 0;
	if (!util::load_obj(model_path + "spike_tile.obj", vbo, ebo, mb_elems, &mb_verts, ma_verts, ma_elems)){
		std::cout << "Failed to load quad\n";
		return 1;
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	vbo.bind();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vbo.stride(), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vbo.stride(), reinterpret_cast<void*>(vbo.offset(1)));

	ebo.bind();

	PackedBuffer<glm::vec3, glm::mat4> attribs{6, GL_ARRAY_BUFFER, GL_STATIC_DRAW};
	attribs.map(GL_WRITE_ONLY);
	//The dented tiles
	attribs.write<0>(0) = glm::vec3{1.f, 0.f, 0.f};
	attribs.write<1>(0) = glm::translate(glm::vec3{-1.f, 0.f, 0.f});
	attribs.write<0>(1) = glm::vec3{1.f, 0.f, 1.f};
	attribs.write<1>(1) = glm::translate(glm::vec3{1.f, 0.f, -2.f});
	attribs.write<0>(2) = glm::vec3{1.f, 0.f, 1.f};
	attribs.write<1>(2) = glm::translate(glm::vec3{-1.f, 0.f, 2.f})
		* glm::rotate(util::deg_to_rad(90), glm::vec3{0, 1, 0});
	//The pointed tiles
	attribs.write<0>(3) = glm::vec3{0.f, 0.f, 1.f};
	attribs.write<1>(3) = glm::translate(glm::vec3{1.f, 0.f, 0.f});
	attribs.write<0>(4) = glm::vec3{1.f, 1.f, 0.f};
	attribs.write<1>(4) = glm::translate(glm::vec3{-1.f, 0.f, -2.f});
	attribs.write<0>(5) = glm::vec3{1.f, 0.f, 0.f};
	attribs.write<1>(5) = glm::translate(glm::vec3{1.f, 0.f, 2.f});
	attribs.unmap();
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, attribs.stride(), 0);
	glVertexAttribDivisor(2, 1);
	for (int i = 3; i < 7; ++i){
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, attribs.stride(),
			reinterpret_cast<void*>(attribs.offset(1) + (i - 3) * sizeof(glm::vec4)));
		glVertexAttribDivisor(i, 1);
	}

	PackedBuffer<DrawElementsIndirectCommand> draw_commands{2, GL_DRAW_INDIRECT_BUFFER, GL_STATIC_DRAW};
	draw_commands.map(GL_WRITE_ONLY);
	//We're drawing two of the first tile type and 3 of the second
	draw_commands.write<0>(0) = DrawElementsIndirectCommand{ma_elems, 3, 0, 0, 0};
	draw_commands.write<0>(1) = DrawElementsIndirectCommand{mb_elems, 3, ma_elems, 0, 3};
	draw_commands.unmap();

	SDL_Event e;
	bool quit = false, view_change = false;
	int view_pos = 0;
	while (!quit){
		while (SDL_PollEvent(&e)){
			if (e.type == SDL_QUIT){
				quit = true;
			}
			else if (e.type == SDL_KEYDOWN){
				switch (e.key.keysym.sym){
					case SDLK_d:
						view_pos = (view_pos + 1) % 4;
						view_change = true;
						break;
					case SDLK_a:
						view_pos = view_pos == 0 ? 3 : (view_pos - 1) % 4;
						view_change = true;
						break;
					case SDLK_ESCAPE:
						quit = true;
						break;
					default:
						break;
				}
			}
		}
		if (view_change){
			view_change = false;
			glm::vec3 eye_pos;
			switch (view_pos){
				case 1:
					eye_pos = glm::vec3{5, 4, 0};
					break;
				case 2:
					eye_pos = glm::vec3{0, 4, -5};
					break;
				case 3:
					eye_pos = glm::vec3{-5, 4, 0};
					break;
				default:
					eye_pos = glm::vec3{0, 4, 5};
					break;
			}
			viewing.map(GL_WRITE_ONLY);
			viewing.write<0>(0) = glm::lookAt(eye_pos, glm::vec3{0.f, 0.f, 0.f},
				glm::vec3{0.f, 1.f, 0.f});
			viewing.unmap();
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL,
				draw_commands.size(), draw_commands.stride());

		SDL_GL_SwapWindow(win);
	}

	glDeleteProgram(shader);
	glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}

