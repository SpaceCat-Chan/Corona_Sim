#pragma once

#include <chrono>

#include <boost/serialization/access.hpp>
#include <glm/ext.hpp>

#include "PathResult.hpp"

struct Action
{
	double when;
	std::pair<int, glm::dvec2> where;
	bool allow_wander = false;

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, unsigned int)
	{
		ar &when;
		ar &where;
		ar &allow_wander;
	}
};

struct Routine
{
	double repeat_interval;
	std::vector<Action> actions;

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, unsigned int)
	{
		ar &repeat_interval;
		ar &actions;
	}
};

class Person
{
	public:
	enum
	{
		susceptible,
		infected,
		recovered,
	} state;

	static const char *StateString(decltype(Person::state) state)
	{
		switch (state)
		{
		case susceptible:
			return "susceptible";
		case infected:
			return "infected";
		case recovered:
			return "recovered";
		}
	}

	Routine routine;
	size_t routine_step = 0;
	double time_offset = 0;

	double infection_finish_time;

	glm::dvec2 position;
	int floor;
	size_t going_to;
	PathResult going_along;
	std::optional<double> switching_floor_time;

	double noise_seed = 3;
	glm::dvec2 current_direction = {0, 1};

	private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive &ar, unsigned int)
	{
		ar &position;
		ar &floor;
		ar &routine;
		ar &state;
		ar &noise_seed;
	}
};
