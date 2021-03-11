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

void World::add_floor(int floor) { m_map.emplace(floor, Floor{}); }

const decltype(World::m_map) &World::get_layout() const { return m_map; }

bool World::test_line_of_sight(
    int floor,
    glm::dvec2 from,
    glm::dvec2 to,
    bool movement,
    bool infection, double expand) const
{
	if (m_map.contains(floor))
	{
		return m_map.at(floor).test_line_of_sight(from, to, movement, infection, expand);
	}
	return true;
}

bool Floor::test_line_of_sight(
    glm::dvec2 from,
    glm::dvec2 to,
    bool movement,
    bool infection, double expand) const
{
	for (auto &obstacle : obstacles)
	{
		if ((movement && obstacle.blocks_movement)
		    || (infection && obstacle.blocks_infection))
		{
			if (obstacle.intersects(from, to, expand))
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

std::vector<glm::dvec2>
Obstacle::get_vertecies(double expand_by, bool include_rotations /*=true*/) const
{
	auto center = (position * 2.0 + size) / 2.0;
	auto rotate = glm::translate(
	    glm::scale(
	        glm::rotate(
	            glm::translate(glm::dmat3{1}, center),
	            include_rotations ? rotation : 0.0),
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

bool simple_point_AABB(glm::dvec2 point, const Obstacle &ob)
{
	auto result = glm::greaterThanEqual(point, ob.position)
	              && glm::lessThanEqual(point, ob.position + ob.size);
	return result.x && result.y;
}

bool Obstacle::intersects(glm::dvec2 point) const
{

	auto center = position + size / 2.0;
	auto rotate = glm::translate(glm::dmat4{1}, glm::dvec3{center, 0})
	              * glm::rotate(glm::dmat4{1}, -rotation, glm::dvec3{0, 0, 1})
	              * glm::translate(glm::dmat4{1}, -glm::dvec3{center, 0});
	return simple_point_AABB(rotate * glm::dvec4{point, 0, 1}, *this);
}

bool Obstacle::intersects(const Obstacle &other) const
{
	auto my_center = position + size / 2.0;
	auto my_rotate = glm::translate(glm::dmat4{1}, glm::dvec3{my_center, 0})
	                 * glm::rotate(glm::dmat4{1}, -rotation, glm::dvec3{0, 0, 1})
	                 * glm::translate(glm::dmat4{1}, -glm::dvec3{my_center, 0});

	auto other_points = other.get_vertecies(0);
	for (auto &point : other_points)
	{
		if (simple_point_AABB(my_rotate * glm::dvec4{point, 0, 1}, *this))
		{
			return true;
		}
	}

	auto other_center = other.position + other.size / 2.0;
	auto other_rotate
	    = glm::translate(glm::dmat4{1}, glm::dvec3{other_center, 0})
	      * glm::rotate(glm::dmat4{1}, -other.rotation, glm::dvec3{0, 0, 1})
	      * glm::translate(glm::dmat4{1}, -glm::dvec3{other_center, 0});

	auto my_points = get_vertecies(0);
	for (auto &point : my_points)
	{
		if (simple_point_AABB(other_rotate * glm::dvec4{point, 0, 1}, other))
		{
			return true;
		}
	}

	return other.intersects(my_points[0], my_points[1])
	       || other.intersects(my_points[1], my_points[2])
	       || other.intersects(my_points[2], my_points[3])
	       || other.intersects(my_points[3], my_points[0]);
}

bool Obstacle::intersects(glm::dvec2 from, glm::dvec2 to, double expand) const
{
	if (intersects(from) || intersects(to))
	{
		return true;
	}
	auto vertecies = get_vertecies(expand);
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

const std::vector<std::pair<glm::dvec2, std::vector<size_t>>> &
Floor::recalc_visibility_graph() const
{
	if (!needs_recalc)
	{
		return visibility_graph;
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
	return visibility_graph;
}

PathResult World::calculate_path(
    int from_floor,
    glm::dvec2 from,
    int to_floor,
    glm::dvec2 to) const
{
	auto floor_pathing = floor_path(from_floor, to_floor);
	if (floor_pathing)
	{
		//true = entering a, false = entering b
		std::vector<std::pair<std::vector<glm::dvec2>, int>> all_path_results;
		std::vector<std::pair<FloorChanger, bool>> all_floor_changers;
		for (size_t i = 0; i < floor_pathing->size(); i++)
		{
			std::vector<glm::dvec2> to_next_floor;
			std::vector<std::pair<FloorChanger, bool>> next_floor_changers;
			if (i != floor_pathing->size() - 1)
			{
				for (auto &changer : floor_changers)
				{
					if (changer.a.first == floor_pathing->at(i)
					    && changer.b.first == floor_pathing->at(i + 1))
					{
						to_next_floor.push_back(changer.a.second);
						next_floor_changers.emplace_back(changer, true);
					}
					if (changer.b.first == floor_pathing->at(i)
					    && changer.a.first == floor_pathing->at(i + 1))
					{
						to_next_floor.push_back(changer.b.second);
						next_floor_changers.emplace_back(changer, false);
					}
				}
			}
			else
			{
				to_next_floor.push_back(to);
			}
			glm::dvec2 start;
			if (i == 0)
			{
				start = from;
			}
			else
			{
				auto changer = all_floor_changers.back();
				if (changer.second)
				{
					start = changer.first.b.second;
				}
				else
				{
					start = changer.first.a.second;
				}
			}
			AStar Pather{
			    start,
			    to_next_floor,
			    [this, &floor_pathing, &i](glm::dvec2 from, glm::dvec2 to) {
				    return test_line_of_sight(
				        floor_pathing->at(i),
				        from,
				        to,
				        true,
				        false);
			    },
			    m_map.at(floor_pathing->at(i)).recalc_visibility_graph()};
			Pather.run();

			auto results = Pather.path_result();
			if (results)
			{
				all_path_results.push_back(*results);
				if (i != floor_pathing->size() - 1)
				{
					all_floor_changers.push_back(
					    next_floor_changers[results->second]);
				}
			}
			else
			{
				PathResult a;
				a.force_teleport = true;
				a.waypoints.emplace_back(std::pair{to_floor, to});
				return a;
			}
		}
		PathResult result;
		for (size_t i = 0; i < all_path_results.size(); i++)
		{
			for (auto &waypoint : all_path_results[i].first)
			{
				result.waypoints.emplace_back(waypoint);
			}
			if (i != floor_pathing->size() - 1)
			{
				if (all_floor_changers[i].second)
				{
					result.waypoints.emplace_back(all_floor_changers[i].first.b);
				}
				else
				{
					result.waypoints.emplace_back(all_floor_changers[i].first.a);
				}
			}
		}
		return result;
	}
	else
	{
		PathResult a;
		a.force_teleport = true;
		a.waypoints.emplace_back(std::pair{to_floor, to});
		return a;
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

std::optional<std::vector<int>> World::floor_path(int from, int to) const
{
	std::unordered_set<int> seen;
	auto result = floor_path(from, to, seen);
	if (result)
	{
		return {{result->rbegin(), result->rend()}};
	}
	return std::nullopt;
}

std::optional<std::vector<int>>
World::floor_path(int from, int to, std::unordered_set<int> &already_seen) const
{
	if (from == to)
	{
		return {{to}};
	}
	already_seen.insert(from);
	std::vector<int> options;
	for (auto &changer : floor_changers)
	{
		if (changer.a.first == from)
		{
			options.push_back(changer.b.first);
		}
		if (changer.b.first == from)
		{
			options.push_back(changer.a.first);
		}
	}
	for (auto option : options)
	{
		if (already_seen.contains(option) || !m_map.contains(option))
		{
			continue;
		}
		auto attempt = floor_path(option, to, already_seen);
		if (attempt)
		{
			attempt->push_back(from);
			return attempt;
		}
	}
	return std::nullopt;
}
