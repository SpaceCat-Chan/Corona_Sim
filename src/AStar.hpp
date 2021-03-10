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
	    std::vector<glm::dvec2> ends,
	    std::function<bool(glm::dvec2, glm::dvec2)> visibility,
	    const std::vector<std::pair<glm::dvec2, std::vector<size_t>>>
	        &visibility_map)
	{
		m_end_locations = ends;
		m_visibility_map = visibility_map;
		auto start_index = m_visibility_map.size();
		std::vector<size_t> end_index;
		m_visibility_map.emplace_back(start, std::vector<size_t>{});
		end_offset = m_visibility_map.size();
		for (auto end : ends)
		{
			m_visibility_map.emplace_back(end, std::vector<size_t>{});
			end_index.push_back(m_visibility_map.size() - 1);
		}
		for (size_t i = 0; i < end_index[0]; i++)
		{
			if (visibility(start, m_visibility_map[i].first) && i != start_index)
			{
				m_visibility_map[start_index].second.push_back(i);
				m_visibility_map[i].second.push_back(start_index);
			}
			for (size_t end = 0; end < ends.size(); end++)
				if (visibility(ends[end], m_visibility_map[i].first))
				{
					m_visibility_map[end_index[end]].second.push_back(i);
					m_visibility_map[i].second.push_back(end_index[end]);
				}
		}

		m_ends = end_index;
		auto start_candidate
		    = std::make_shared<PathingNode>(start, ends, start_index);
		m_f_open_nodes.insert({start_candidate->f, start_candidate});
		m_pos_open_nodes.insert({start_index, start_candidate});
	}

	bool run()
	{
		while (!stop)
		{
			for (auto &end : m_ends)
			{
				if (m_closed_nodes.find(end) != m_closed_nodes.end())
				{
					return true;
				}
			}
			if (m_f_open_nodes.empty())
			{
				return false;
			}
			single_iteration();
		}
		return false;
	}

	std::optional<std::pair<std::vector<glm::dvec2>, size_t>> path_result()
	{
		std::vector<glm::dvec2> reverse_result;
		decltype(m_closed_nodes)::iterator end_iter;
		for (auto &end : m_ends)
		{
			auto attempt = m_closed_nodes.find(end);
			if (attempt != m_closed_nodes.end())
			{
				end_iter = attempt;
				break;
			}
		}
		if (end_iter == m_closed_nodes.end())
		{
			return std::nullopt;
		}
		auto end = end_iter->second.get();
		while (end != nullptr)
		{
			reverse_result.push_back(end->position);
			end = end->parent;
		}
		return std::pair{
		    std::vector<glm::dvec2>{
		        reverse_result.rbegin(),
		        reverse_result.rend()},
		    end_iter->first - end_offset};
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
				if (already->second->attempt_new_parent(candidate->second.get()))
				{
					std::erase_if(m_f_open_nodes, [b = already->second](auto a) {
						return a.second == b;
					});
					m_f_open_nodes.insert({already->second->f, already->second});
				}
			}
			else
			{
				auto constructed_candidate = std::make_shared<PathingNode>(
				    candidate->second.get(),
				    m_visibility_map[new_candidate].first,
				    m_end_locations,
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

	size_t end_offset;
	std::vector<glm::dvec2> m_end_locations;
	std::vector<size_t> m_ends;
};
