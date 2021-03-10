#pragma once

#include <chrono>

#include <glm/ext.hpp>

#include "PathResult.hpp"

class Person
{
	public:
	enum
	{
		susceptible,
		infected,
		recovered,
	} state;

	static const char* StateString(decltype(Person::state) state)
	{
		switch(state)
		{
			case susceptible:
				return "susceptible";
			case infected:
				return "infected";
			case recovered:
				return "recovered";
		}
	}

	//TODO: replace type with a different one that actually contains routine info
	std::optional<std::pair<int, glm::dvec2>> Routine;

	double infection_finish_time;

	glm::dvec2 position;
	int floor;
	size_t going_to;
	PathResult going_along;
	std::optional<double> switching_floor_time;
};
