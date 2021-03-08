#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>

#include <glm/ext.hpp>

#include "PathingNode.hpp"

class AStar
{
	public:
	AStar(
	    glm::dvec2 start,
	    glm::dvec2 end,
	    std::function<bool(glm::dvec2, glm::dvec2)> visibility,
	    const std::vector<std::pair<glm::dvec2, std::vector<size_t>>>
	        &visibility_map)
	{
		m_visibility_map = visibility_map;
		auto start_index = m_visibility_map.size();
		auto end_index = m_visibility_map.size() + 1;
		m_visibility_map.emplace_back(start, std::vector<size_t>{});
		m_visibility_map.emplace_back(end, std::vector<size_t>{});

		for (size_t i = 0; i < end_index; i++)
		{
			if (visibility(start, m_visibility_map[i].first) && i != start_index)
			{
				m_visibility_map[start_index].second.push_back(i);
				m_visibility_map[i].second.push_back(start_index);
			}
			if (visibility(end, m_visibility_map[i].first))
			{
				m_visibility_map[end_index].second.push_back(i);
				m_visibility_map[i].second.push_back(end_index);
			}
		}

		m_end = end_index;
		auto start_candidate
		    = std::make_shared<PathingNode>(start, end, start_index);
		m_f_open_nodes.insert({start_candidate->f, start_candidate});
		m_pos_open_nodes.insert({start_index, start_candidate});
	}

	bool run()
	{
		while (!stop)
		{
			if (m_closed_nodes.find(m_end) != m_closed_nodes.end())
			{
				return true;
			}
			if (m_f_open_nodes.empty())
			{
				return false;
			}
			single_iteration();
		}
		return false;
	}

	std::vector<glm::dvec2> path_result()
	{
		std::vector<glm::dvec2> reverse_result;
		auto end = m_closed_nodes.find(m_end)->second.get();
		while (end != nullptr)
		{
			reverse_result.push_back(end->position);
			end = end->parent;
		}
		return {reverse_result.rbegin(), reverse_result.rend()};
	}

	bool stop = false;

	private:
	void single_iteration()
	{
		auto candidate = m_f_open_nodes.begin();
		std::vector<size_t> new_candidates
		    = m_visibility_map[candidate->second->index].second;

		for (auto &new_candidate : new_candidates)
		{
			if (m_closed_nodes.contains(new_candidate))
			{
				continue;
			}
			if (auto already = m_pos_open_nodes.find(new_candidate);
			    already != m_pos_open_nodes.end())
			{
				if (already->second->g > glm::distance(
				                             candidate->second->position,
				                             already->second->position)
				                             + candidate->second->g)
				{
					already->second->new_parent(candidate->second.get());
				}
			}
			else
			{
				auto constructed_candidate = std::make_shared<PathingNode>(
				    candidate->second.get(),
				    m_visibility_map[new_candidate].first,
				    m_visibility_map[m_end].first,
				    new_candidate);
				m_f_open_nodes.insert(
				    {constructed_candidate->f, constructed_candidate});
				m_pos_open_nodes.insert({new_candidate, constructed_candidate});
			}
		}
		m_closed_nodes.insert({candidate->second->index, candidate->second});
		m_f_open_nodes.erase(candidate);
		m_pos_open_nodes.erase(candidate->first);
	}

	std::multimap<double, std::shared_ptr<PathingNode>> m_f_open_nodes;
	std::unordered_map<size_t, std::shared_ptr<PathingNode>> m_pos_open_nodes;
	std::unordered_map<size_t, std::shared_ptr<PathingNode>> m_closed_nodes;

	std::vector<std::pair<glm::dvec2, std::vector<size_t>>> m_visibility_map;

	size_t m_end;
};