#include "world.hpp"

#include "AStar.hpp"

void World::add_obstacle(int floor, Obstacle obstacle)
{
	if (m_map.contains(floor))
	{
		m_map.at(floor).obstacles.push_back(obstacle);
		m_map.at(floor).recalc();
	}
}

void World::remove_obstacle(int floor, int obstacle)
{
	if (m_map.contains(floor))
	{
		m_map[floor].obstacles.erase(m_map[floor].obstacles.begin() + obstacle);
		m_map[floor].recalc();
	}
}

void World::remove_floor(int floor) { m_map.erase(floor); }

void World::add_floor(int floor) {m_map.emplace(floor, Floor{}); }

const decltype(World::m_map) &World::get_layout() const { return m_map; }

bool World::test_line_of_sight(
    int floor,
    glm::dvec2 from,
    glm::dvec2 to,
    bool movement,
    bool infection) const
{
	if (m_map.contains(floor))
	{
		return m_map.at(floor).test_line_of_sight(from, to, movement, infection);
	}
	return true;
}

bool Floor::test_line_of_sight(
    glm::dvec2 from,
    glm::dvec2 to,
    bool movement,
    bool infection) const
{
	for (auto &obstacle : obstacles)
	{
		if ((movement && obstacle.blocks_movement)
		    || (infection && obstacle.blocks_infection))
		{
			if (obstacle.intersects(from, to))
			{
				return false;
			}
		}
	}
	return true;
}
inline double Det(double a, double b, double c, double d)
{
	return a * d - b * c;
}

bool LineLineIntersect(
    double x1,
    double y1, //Line 1 start
    double x2,
    double y2, //Line 1 end
    double x3,
    double y3, //Line 2 start
    double x4,
    double y4, //Line 2 end
    double &ixOut,
    double &iyOut) //Output
{
	//http://mathworld.wolfram.com/Line-LineIntersection.html

	double detL1 = Det(x1, y1, x2, y2);
	double detL2 = Det(x3, y3, x4, y4);
	double x1mx2 = x1 - x2;
	double x3mx4 = x3 - x4;
	double y1my2 = y1 - y2;
	double y3my4 = y3 - y4;

	double xnom = Det(detL1, x1mx2, detL2, x3mx4);
	double ynom = Det(detL1, y1my2, detL2, y3my4);
	double denom = Det(x1mx2, y1my2, x3mx4, y3my4);
	if (denom == 0.0) //Lines don't seem to cross
	{
		ixOut = NAN;
		iyOut = NAN;
		return false;
	}

	ixOut = xnom / denom;
	iyOut = ynom / denom;
	if (!std::isfinite(ixOut)
	    || !std::isfinite(iyOut)) //Probably a numerical issue
		return false;

	if (is_within(ixOut, x1, x2) && is_within(ixOut, x3, x4)
	    && is_within(iyOut, y1, y2) && is_within(iyOut, y3, y4))
	{
		return true; //All OK
	}

	return false;
}

std::vector<glm::dvec2> Obstacle::get_vertecies(double expand_by, bool include_rotations/*=true*/) const
{
	auto center = (position * 2.0 + size) / 2.0;
	auto rotate = glm::translate(
	    glm::scale(
	        glm::rotate(glm::translate(glm::dmat3{1}, center), include_rotations ? rotation : 0.0),
	        glm::dvec2{
	            ((center - position).x + expand_by) / (center - position).x,
	            ((center - position).y + expand_by) / (center - position).y}),
	    -center);
	return {
	    glm::dvec2{rotate * glm::dvec3{position, 1}},
	    glm::dvec2{rotate * glm::dvec3{position + glm::dvec2{size.x, 0}, 1}},
	    glm::dvec2{rotate * glm::dvec3{position + size, 1}},
	    glm::dvec2{rotate * glm::dvec3{position + glm::dvec2{0, size.y}, 1}}};
}

bool point_inside_rect(glm::dvec2 point, Obstacle rect)
{
	auto result = glm::greaterThanEqual(point, rect.position)
	              && glm::lessThanEqual(point, rect.position + rect.size);
	return result.x && result.y;
}

bool Obstacle::intersects(glm::dvec2 from, glm::dvec2 to) const
{
	if (point_inside_rect(from, *this) || point_inside_rect(to, *this))
	{
		return true;
	}
	auto vertecies = get_vertecies(0);
	double dont_care, dont_care_2;
	return LineLineIntersect(
	           from.x,
	           from.y,
	           to.x,
	           to.y,
	           vertecies[0].x,
	           vertecies[0].y,
	           vertecies[1].x,
	           vertecies[1].y,
	           dont_care,
	           dont_care_2)
	       || LineLineIntersect(
	           from.x,
	           from.y,
	           to.x,
	           to.y,
	           vertecies[1].x,
	           vertecies[1].y,
	           vertecies[2].x,
	           vertecies[2].y,
	           dont_care,
	           dont_care_2)
	       || LineLineIntersect(
	           from.x,
	           from.y,
	           to.x,
	           to.y,
	           vertecies[2].x,
	           vertecies[2].y,
	           vertecies[3].x,
	           vertecies[3].y,
	           dont_care,
	           dont_care_2)
	       || LineLineIntersect(
	           from.x,
	           from.y,
	           to.x,
	           to.y,
	           vertecies[3].x,
	           vertecies[3].y,
	           vertecies[0].x,
	           vertecies[0].y,
	           dont_care,
	           dont_care_2);
}

void Floor::recalc_visibility_graph() const
{
	if (!needs_recalc)
	{
		return;
	}
	visibility_graph.clear();
	for (auto &obstacle : obstacles)
	{
		auto vertecies = obstacle.get_vertecies(0.011);
		visibility_graph.emplace_back(vertecies[0], std::vector<size_t>{});
		visibility_graph.emplace_back(vertecies[1], std::vector<size_t>{});
		visibility_graph.emplace_back(vertecies[2], std::vector<size_t>{});
		visibility_graph.emplace_back(vertecies[3], std::vector<size_t>{});
	}

	for (size_t i = 0; i < visibility_graph.size(); ++i)
	{
		auto &vertex = visibility_graph[i];
		for (size_t j = i + 1; j < visibility_graph.size(); ++j)
		{
			auto &other_vertex = visibility_graph[j];
			if (test_line_of_sight(
			        vertex.first,
			        other_vertex.first,
			        true,
			        false))
			{
				vertex.second.push_back(j);
				other_vertex.second.push_back(i);
			}
		}
	}
	needs_recalc = false;
}

PathResult Floor::calculate_path(glm::dvec2 from, glm::dvec2 to) const
{
	recalc_visibility_graph();
	AStar Pather{
	    from,
	    to,
	    [this](glm::dvec2 from, glm::dvec2 to) {
		    return test_line_of_sight(from, to, true, false);
	    },
	    visibility_graph};

	Pather.run();
	return {Pather.path_result()};
}

PathResult World::calculate_path(
    int from_floor,
    glm::dvec2 from,
    int to_floor,
    glm::dvec2 to) const
{
	if (m_map.contains(from_floor))
	{
		return m_map.at(from_floor).calculate_path(from, to);
	}
	else
	{
		return {};
	}
}

Obstacle &World::get_obstacle(int floor, size_t index)
{
	return m_map.at(floor).obstacles.at(index);
}

const Obstacle &World::get_obstacle(int floor, size_t index) const
{
	return m_map.at(floor).obstacles.at(index);
}

void World::set_floor_name(int floor, std::string name)
{
	m_map.at(floor).name = name;
}
void World::set_floor_group(int floor, std::string group)
{
	m_map.at(floor).group = group;
}

std::vector<int> World::floor_path(int from, int to)
{
	std::vector<int> seen;
	return floor_path(from, to, seen);
}

std::vector<int> World::floor_path(int from, int to, std::vector<int>& already_seen)
{
	
}