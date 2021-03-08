#pragma once

#include <unordered_map>
#include <vector>

#include <boost/archive/basic_text_iarchive.hpp>
#include <boost/archive/basic_text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>

#include <glm/ext.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#include "PathResult.hpp"

struct Obstacle
{
	glm::dvec2 position;
	glm::dvec2 size;
	double rotation = 0;
	bool blocks_infection = true;
	bool blocks_movement = true;
	std::vector<glm::dvec2> get_vertecies(double expand_by) const;
	bool intersects(glm::dvec2 from, glm::dvec2 to) const;

	Obstacle() = default;
	Obstacle(glm::dvec2 position_, glm::dvec2 size_, double rotation_)
	{
		position = position_;
		size = size_;
		rotation = rotation_;
	}
};



struct Floor
{
	std::vector<Obstacle> obstacles;
	bool test_line_of_sight(glm::dvec2 pos_a, glm::dvec2 pos_b, bool movement, bool infection) const;
	PathResult calculate_path(glm::dvec2 from, glm::dvec2 to) const;
	void recalc_visibility_graph() const;
	void recalc() const
	{
		needs_recalc = true;
	}
	private:
	mutable bool needs_recalc = false;
	mutable std::vector<std::pair<glm::dvec2, std::vector<size_t>>> visibility_graph;
};

class World
{
	std::unordered_map<int, Floor> m_map;

	public:
	bool test_line_of_sight(int floor, glm::dvec2 pos_a, glm::dvec2 pos_b, bool movement, bool infection) const;

	PathResult
	calculate_path(int from_floor, glm::dvec2 from, int to_floor, glm::dvec2 to)
	    const;

	void add_obstacle(int floor, Obstacle);
	void remove_obstacle(int floor, int obstacle);
	void remove_floor(int floor);
	auto get_layout() const -> const decltype(m_map) &;

	private:
	friend class boost::serialization::access;

	template <typename Archive>
	void serialize(Archive ar, unsigned int version)
	{
		ar &m_map;
		glm::abs(version);
	}
};
