#pragma once

#include <variant>

struct PathResult
{
	//a series of places that must be moved to in order
	std::vector<std::variant<glm::dvec2, std::pair<int, glm::dvec2>>> waypoints;
	bool force_teleport = false;
	PathResult() = default;
	PathResult(const std::vector<glm::dvec2>& move_from)
	{
		for(auto& a : move_from)
		{
			waypoints.emplace_back(a);
		}
	}
};
