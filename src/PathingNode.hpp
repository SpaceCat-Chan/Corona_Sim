#pragma once

#include <glm/ext.hpp>

struct PathingNode
{
	PathingNode *parent = nullptr;
	glm::dvec2 position;
	size_t index;
	double g = 0, h = 0, f = 0;

	PathingNode(
	    PathingNode *_parent,
	    glm::dvec2 new_position,
	    std::vector<glm::dvec2> end_positions,
	    size_t index_)
	{
		update_distance(new_position, end_positions);
		new_parent(_parent);
		index = index_;
	}
	PathingNode(
	    glm::dvec2 new_position,
	    std::vector<glm::dvec2> end_positions,
	    size_t index_)
	{
		update_distance(new_position, end_positions);
		index = index_;
	}
	void update_distance(
	    glm::dvec2 new_position,
	    std::vector<glm::dvec2> end_positions)
	{
		position = new_position;
		double min = 1e100;
		for (auto &end : end_positions)
		{
			auto dist = glm::distance(new_position, end);
			if (dist < min)
			{
				min = dist;
			}
		}
		h = min;
		f = g + h;
	}
	void new_parent(PathingNode *_parent)
	{
			parent = _parent;
			g = glm::distance(parent->position, position) + parent->g;
			f = g + h;
	}
	bool attempt_new_parent(PathingNode *_parent)
	{
		if ((glm::distance(_parent->position, position) + _parent->g) < g)
		{
			parent = _parent;
			g = glm::distance(parent->position, position) + parent->g;
			f = g + h;
			return true;
		}
		return false;
	}
};
