#ifndef MULTI_RENDERBATCH_H
#define MULTI_RENDERBATCH_H

#include <iostream>
#include <tuple>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include "gl_core_4_4.h"
#include "glattrib_type.h"
#include "interleavedbuffer.h"
#include "renderbatch.h"
#include "model.h"

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

/*
 * Implements instanced rendering of multiple objects through glMultiDrawElementsIndirect
 */
template<typename... Attribs>
class MultiRenderBatch {
	//Sizes of the batches for each model, the number of models we can fit before hitting the next batch's
	//attributes and offsets in the attributes buffer for each batch
	std::vector<size_t> batch_capacities, batch_sizes, batch_offsets;
	//The models being drawn by the batch packed into a single buffer
	PackedBuffer<glm::vec3, glm::vec3, glm::vec3> model_vbo;
	PackedBuffer<GLushort> model_ebo;
	InterleavedBuffer<Layout::PACKED, Attribs...> attributes;
	PackedBuffer<DrawElementsIndirectCommand> draw_commands;
	std::array<int, sizeof...(Attribs)> indices;
	GLuint vao;

public:
	/*
	 * Create the multi render batch to handle drawing the models that have been packed into
	 * the vbo and ebo passed. Also pass in the desired sizes for each batch and offsets
	 * in the packed models buffer to their elements
	 */
	MultiRenderBatch(const std::vector<size_t> batch_capacities, const std::vector<size_t> &model_elems,
		const std::vector<size_t> &model_elem_offsets, PackedBuffer<glm::vec3, glm::vec3, glm::vec3> &&model_vbo,
		PackedBuffer<GLushort> &&model_ebo);
	/*
	 * Get access to the underlying attributes buffer
	 */
	InterleavedBuffer<Layout::PACKED, Attribs...>& attrib_buf();
	/*
	 * Push an instance of one of the models to be drawn
	 */
	void push_instance(size_t model, const std::tuple<Attribs...> &a);
	/*
	 * Set the attribute index to send the attributes too
	 */
	void set_attrib_indices(const std::array<int, sizeof...(Attribs)> &i);
	/*
	 * Render the multi batch
	 */
	void render();

private:
	/*
	 * Recurse through the types in the attribute buffer and set their indices
	 */
	template<typename T>
	void set_attrib_index();
	template<typename A, typename B, typename... Args>
	void set_attrib_index();
};

template<typename... Attribs>
MultiRenderBatch<Attribs...>::MultiRenderBatch(const std::vector<size_t> batch_capacities, const std::vector<size_t> &model_elems,
	const std::vector<size_t> &model_elem_offsets, PackedBuffer<glm::vec3, glm::vec3, glm::vec3> &&vbo,
	PackedBuffer<GLushort> &&ebo)
	: batch_capacities(batch_capacities), batch_sizes(batch_capacities.size(), 0), model_vbo(std::move(vbo)), model_ebo(std::move(ebo)),
	attributes(std::accumulate(batch_capacities.begin(), batch_capacities.end(), 0), GL_ARRAY_BUFFER, GL_STREAM_DRAW),
	draw_commands(batch_capacities.size(), GL_DRAW_INDIRECT_BUFFER, GL_STATIC_DRAW)
{
	batch_offsets.reserve(batch_capacities.size());
	batch_offsets.push_back(0);
	std::copy(batch_capacities.begin(), batch_capacities.end() - 1, std::back_inserter(batch_offsets));
	//Hook up the model vao using the regular indices I use for position and normal
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	model_vbo.bind();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, model_vbo.stride(), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, model_vbo.stride(), reinterpret_cast<void*>(model_vbo.offset(1)));
	model_ebo.bind();

	draw_commands.map(GL_WRITE_ONLY);
	for (size_t i = 0; i < batch_capacities.size(); ++i){
		draw_commands.write<0>(i) = DrawElementsIndirectCommand{static_cast<GLuint>(model_elems[i]), 0,
			static_cast<GLuint>(model_elem_offsets[i]), 0, static_cast<GLuint>(batch_offsets[i])};
	}
	draw_commands.unmap();
}
template<typename... Attribs>
InterleavedBuffer<Layout::PACKED, Attribs...>& MultiRenderBatch<Attribs...>::attrib_buf(){
	return attributes;
}
template<typename... Attribs>
void MultiRenderBatch<Attribs...>::push_instance(size_t model, const std::tuple<Attribs...> &a){
	assert(batch_sizes[model] + 1 <= batch_capacities[model]);
	//Write the attribute for this new instance of the model and update batch size
	attributes.map_range(batch_offsets[model] + batch_sizes[model], 1, GL_MAP_WRITE_BIT);
	attributes.write(batch_offsets[model] + batch_sizes[model], a);
	attributes.unmap();
	++batch_sizes[model];

	//Update our draw command for this batch
	draw_commands.map_range(model, 1, GL_MAP_WRITE_BIT);
	auto &cmd = draw_commands.write<0>(model);
	++cmd.instance_count;
	draw_commands.unmap();
}
template<typename... Attribs>
void MultiRenderBatch<Attribs...>::set_attrib_indices(const std::array<int, sizeof...(Attribs)> &i){
	indices = i;
	glBindVertexArray(vao);
	attributes.bind();
	set_attrib_index<Attribs...>();
}
template<typename... Attribs>
void MultiRenderBatch<Attribs...>::render(){
	glBindVertexArray(vao);
	draw_commands.bind();
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, draw_commands.size(),
		draw_commands.stride());
}
template<typename... Attribs>
template<typename T>
void MultiRenderBatch<Attribs...>::set_attrib_index(){
	int index = sizeof...(Attribs) - 1;
	size_t base_offset = attributes.offset(index);
	GLenum gl_type = detail::gl_attrib_type<T>();
	//number of occupied indices is rounded based on vec4
	//would we want to use the Sizer for this?
	size_t attrib_size = detail::Size<Layout::PACKED, T>::size();
	size_t num_indices = attrib_size / sizeof(glm::vec4);
	num_indices = attrib_size % sizeof(glm::vec4) == 0 ? num_indices : num_indices + 1;
	for (size_t i = 0; i < num_indices; ++i){
		glEnableVertexAttribArray(i + indices[index]);
		if (gl_type == GL_FLOAT || gl_type == GL_HALF_FLOAT || gl_type == GL_DOUBLE){
			//TODO: How should we work through computing the number of values we're sending?
			//or is just saying 4 fine
			glVertexAttribPointer(i + indices[index], 4, gl_type, GL_FALSE, attributes.stride(),
					(void*)(base_offset + sizeof(glm::vec4) * i));
		}
		else {
			glVertexAttribIPointer(i + indices[index], 4, gl_type, attributes.stride(),
					(void*)(base_offset + sizeof(glm::vec4) * i));
		}
		glVertexAttribDivisor(i + indices[index], 1);
	}
}
template<typename... Attribs>
template<typename A, typename B, typename... Args>
void MultiRenderBatch<Attribs...>::set_attrib_index(){
	int index = sizeof...(Attribs) - sizeof...(Args) - 2;
	size_t base_offset = attributes.offset(index);
	GLenum gl_type = detail::gl_attrib_type<A>();
	//number of occupied indices is rounded based on vec4
	//would we want to use the Sizer for this?
	size_t attrib_size = detail::Size<Layout::PACKED, A>::size();
	size_t num_indices = attrib_size / sizeof(glm::vec4);
	num_indices = attrib_size % sizeof(glm::vec4) == 0 ? num_indices : num_indices + 1;
	for (size_t i = 0; i < num_indices; ++i){
		glEnableVertexAttribArray(i + indices[index]);
		if (gl_type == GL_FLOAT || gl_type == GL_HALF_FLOAT || gl_type == GL_DOUBLE){
			//TODO: How should we work through computing the number of values we're sending?
			//or is just saying 4 fine
			glVertexAttribPointer(i + indices[index], 4, gl_type, GL_FALSE, attributes.stride(),
					(void*)(base_offset + sizeof(glm::vec4) * i));
		}
		else {
			glVertexAttribIPointer(i + indices[index], 4, gl_type, attributes.stride(),
					(void*)(base_offset + sizeof(glm::vec4) * i));
		}
		glVertexAttribDivisor(i + indices[index], 1);
		//Check that we didn't spill over into another attributes index space
		if (static_cast<int>(i) + indices[index] >= indices[index + 1]){
			std::cerr << "MultiRenderBatch Warning: attribute " << indices[index]
				<< " spilled over into attribute " << indices[index + 1] << std::endl;
		}
	}
	set_attrib_index<B, Args...>();
}

#endif

