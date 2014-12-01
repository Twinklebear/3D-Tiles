#ifndef MULTI_RENDERBATCH_H
#define MULTI_RENDERBATCH_H

#include <iostream>
#include <tuple>
#include <vector>
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
	std::vector<size_t> batch_sizes, batch_capacities, batch_offsets;
	//Number of elements for each model along with the offsets in the ebo to the model
	std::vector<size_t> model_elems, model_elem_offsets;
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
	 * Set the attribute index to send the attributes too
	 */
	void set_attrib_indices(const std::array<int, sizeof...(Attribs)> &i);
	/*
	 * Render the multi batch
	 */
	void render() const;

private:
	/*
	 * Recurse through the types in the attribute buffer and set their indices
	 */
	template<typename T>
	void set_attrib_index();
	template<typename A, typename B, typename... Args>
	void set_attrib_index();
};

#endif

