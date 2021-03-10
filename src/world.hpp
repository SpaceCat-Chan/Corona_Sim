#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
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
	std::vector<glm::dvec2>
	get_vertecies(double expand_by, bool include_rotations = true) const;
	bool intersects(glm::dvec2 from, glm::dvec2 to) const;
	bool intersects(glm::dvec2 point) const;
	bool intersects(const Obstacle &other) const;

	Obstacle() = default;
	Obstacle(glm::dvec2 position_, glm::dvec2 size_, double rotation_)
	{
		position = position_;
		size = size_;
		rotation = rotation_;
	}

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive & ar, unsigned int)
	{
		ar &position;
		ar &size;
		ar &rotation;
		ar &blocks_infection;
		ar &blocks_movement;
	}
};

struct FloorChanger
{
	std::pair<int, glm::dvec2> a, b;
};

struct Floor
{
	std::string name;
	std::string group;
	std::vector<Obstacle> obstacles;
	bool test_line_of_sight(
	    glm::dvec2 pos_a,
	    glm::dvec2 pos_b,
	    bool movement,
	    bool infection) const;
	const std::vector<std::pair<glm::dvec2, std::vector<size_t>>> &
	recalc_visibility_graph() const;
	void recalc() const { needs_recalc = true; }

	private:
	mutable bool needs_recalc = false;
	mutable std::vector<std::pair<glm::dvec2, std::vector<size_t>>>
	    visibility_graph;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive & ar, unsigned int)
	{
		ar &name;
		ar &group;
		ar &obstacles;
		ar &needs_recalc;
		if(!needs_recalc)
		{
			ar &visibility_graph;
		}
	}
};

class World
{
	std::unordered_map<int, Floor> m_map;

	std::vector<FloorChanger> floor_changers;

	public:
	bool test_line_of_sight(
	    int floor,
	    glm::dvec2 pos_a,
	    glm::dvec2 pos_b,
	    bool movement,
	    bool infection) const;

	PathResult
	calculate_path(int from_floor, glm::dvec2 from, int to_floor, glm::dvec2 to)
	    const;

	FloorChanger &get_floor_changer(size_t index)
	{
		return floor_changers.at(index);
	}
	const std::vector<FloorChanger> &get_floor_changers() const
	{
		return floor_changers;
	}
	size_t add_floor_changer(FloorChanger a)
	{
		floor_changers.push_back(a);
		return floor_changers.size() - 1;
	}
	void remove_floor_changer(size_t index)
	{
		floor_changers.erase(floor_changers.begin() + index);
	}

	void add_obstacle(int floor, Obstacle);
	void remove_obstacle(int floor, int obstacle);
	Obstacle &get_obstacle(int floor, size_t obstacle);
	const Obstacle &get_obstacle(int floor, size_t obstacle) const;
	void remove_floor(int floor);
	void add_floor(int floor);
	auto get_layout() const -> const decltype(m_map) &;
	void set_floor_name(int floor, std::string name);
	void set_floor_group(int floor, std::string group);

	private:
	std::optional<std::vector<int>> floor_path(int from, int to) const;
	std::optional<std::vector<int>>
	floor_path(int from, int to, std::unordered_set<int> &already_seen) const;

	friend class boost::serialization::access;

	template <typename Archive>
	void serialize(Archive &ar, unsigned int)
	{
		ar &m_map;
	}
};

inline bool is_within(double val, double a, double b)
{
	constexpr double EPSILON = 0.00001;
	if (a > b)
	{
		std::swap(a, b);
	}
	return a - val <= EPSILON && b - val >= -EPSILON;
}
