#pragma once

#include <glm/ext.hpp>

struct PathingNode
{
	PathingNode *parent = nullptr;
	glm::dvec2 position;
	size_t index;
	double g = 0, h = 0, f = 0;

	PathingNode(PathingNode *_parent, glm::dvec2 new_position, glm::dvec2 end_position, size_t index_)
	{
		new_parent(_parent);
		update_distance(new_position, end_position);
		index = index_;
	}
	PathingNode(glm::dvec2 new_position, glm::dvec2 end_position, size_t index_)
	{
		update_distance(new_position, end_position);
		index = index_;
	}
	void update_distance(glm::dvec2 new_position, glm::dvec2 end_position)
	{
		position = new_position;
		h = glm::distance(new_position, end_position);
		f = g + h;
	}
	void new_parent(PathingNode *_parent)
	{
		parent = _parent;
		g = glm::distance(parent->position, position);
		f = g + h;
	}
};