#pragma once

#include <variant>

struct PathResult
{
	enum class up_or_down
	{
		up,
		down
	};
	//a series of places that must be moved to in order
	std::vector<std::variant<glm::dvec2, up_or_down>> waypoints;
	PathResult() = default;
	PathResult(const std::vector<glm::dvec2>& move_from)
	{
		for(auto& a : move_from)
		{
			waypoints.emplace_back(a);
		}
	}
};
