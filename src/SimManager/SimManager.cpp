#include "SimManager.hpp"

#include <fstream>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <sstream>

#include "glm_serialize.hpp"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

SimManager::SimManager(std::function<double(std::optional<double>)> timescale)
{
	m_timescale = timescale;

	m_world.add_floor(1);
	m_world.add_obstacle(1, {{0.1, 0.4}, {0.8, 0.2}, 0.0});

	for (int i = 0; i < 1; i++)
	{
		m_simulation_start_people.emplace_back();
		m_simulation_start_people[i].state = Person::susceptible;
		m_simulation_start_people[i].position = {0.5, 0.3};
		m_simulation_start_people[i].floor = 1;
	}
}

void SimManager::SimStep(double dt)
{
	if (SimRunning)
	{
		MoveStep(dt);
		InfectStep(dt);
		sim_time += dt;
	}
}

void SimManager::MoveStep(double dt)
{
	for (auto &person : m_current_people)
	{
		if (person.going_to >= person.going_along.waypoints.size())
		{
			//temporary routine, just for testing
			if (person.routine.actions.size() == 0)
			{
				continue;
			}
			if (person.routine_step >= person.routine.actions.size())
			{
				person.routine_step = 0;
			}
			auto current_action = person.routine.actions[person.routine_step];
			auto current_time = sim_time;
			int rounded_time = current_time;
			current_time -= rounded_time;
			rounded_time %= person.routine.repeat_interval;
			current_time += rounded_time;
			if (current_time > current_action.when)
			{
				person.going_along = m_world.calculate_path(
				    person.floor,
				    person.position,
				    current_action.where.first,
				    current_action.where.second);
				person.going_to = 0;
				person.routine_step++;
			}
			continue;
		}

		if (person.going_along.force_teleport)
		{
			auto final = std::get<1>(person.going_along.waypoints.back());
			person.floor = final.first;
			person.position = final.second;
			person.going_to = person.going_along.waypoints.size();
		}

		auto &current_waypoint = person.going_along.waypoints[person.going_to];
		if (current_waypoint.index() == 0)
		{
			auto move_direction
			    = std::get<0>(person.going_along.waypoints[person.going_to])
			      - person.position;
			if (glm::length(glm::normalize(move_direction) * dt * 0.05)
			    < glm::length(move_direction))
			{
				person.position += glm::normalize(move_direction) * dt * 0.05;
			}
			else
			{
				person.position
				    = std::get<0>(person.going_along.waypoints[person.going_to]);
				person.going_to++;
			}
		}
		else
		{
			if (person.switching_floor_time)
			{
				if (sim_time > person.switching_floor_time)
				{
					auto telepot_to = std::get<1>(current_waypoint);
					person.floor = telepot_to.first;
					person.position = telepot_to.second;
					person.going_to++;
					person.switching_floor_time = std::nullopt;
				}
			}
			else
			{
				person.switching_floor_time = sim_time + 2;
			}
		}
	}
}

void SimManager::InfectStep(double dt)
{
	for (size_t first_person_iter = 0;
	     first_person_iter < m_current_people.size();
	     first_person_iter++)
	{
		auto &first_person = m_current_people[first_person_iter];
		if (first_person.state != Person::infected)
		{
			continue;
		}
		for (size_t second_person_iter = 0;
		     second_person_iter < m_current_people.size();
		     second_person_iter++)
		{
			auto &second_person = m_current_people[second_person_iter];
			if (first_person_iter == second_person_iter
			    || second_person.state != Person::susceptible
			    || first_person.floor != second_person.floor
			    || !m_world.test_line_of_sight(
			        first_person.floor,
			        first_person.position,
			        second_person.position,
			        false,
			        true))
			{
				continue;
			}
			auto distance
			    = glm::distance(first_person.position, second_person.position);
			if (distance > minimum_infection_range)
			{
				continue;
			}
			distance *= infection_distance_multiplier;
			distance += 1;
			auto chance = 1 / distance * infection_chance * dt;
			std::uniform_real_distribution dist{0.0, 1.0};
			if (dist(rng) <= chance)
			{
				second_person.state = Person::infected;
				std::uniform_real_distribution infect_time_dist{
				    min_infection_duration,
				    max_infection_duration};
				second_person.infection_finish_time
				    = sim_time + infect_time_dist(rng);
			}
		}
	}

	for (auto &person : m_current_people)
	{
		if (person.state == Person::infected
		    && sim_time > person.infection_finish_time)
		{
			person.state = Person::recovered;
		}
	}
}

//you would expect ui to be done by the renderer, but this is ImGui
//where the rendering and the ui creation are seperated
void SimManager::DrawUI(glm::dvec2 mouse_location)
{
	ImGui::Begin("Simulation Controller");
	if (ImGui::TreeNode("View Settings"))
	{
		float wheel_sens = mousewheel_sensitivity * 100;
		ImGui::SliderFloat(
		    "Mouse wheel sensitivity",
		    &wheel_sens,
		    1,
		    100,
		    "%.3f",
		    ImGuiSliderFlags_Logarithmic);
		mousewheel_sensitivity = wheel_sens / 100.0;
		ImGui::TreePop();
	}
	if (SimRunning)
	{
		ImGui::Text("Simulation is running");
	}
	else
	{
		ImGui::Text("Simulation is not running");
	}
	if (ImGui::Button("(Re)Start Simulation"))
	{
		SimRunning = true;
		m_current_people = m_simulation_start_people;
		sim_time = 0;
		if (m_selection_box)
		{
			if (m_selection_box->size() != 1)
			{
				m_selection_box = std::nullopt;
			}
		}
	}
	if (ImGui::Button("Stop Simulation"))
	{
		SimRunning = false;
	}

	if (viewing_floor_or_group.index() == 0)
	{
		if (ImGui::Button("Create Person"))
		{
			CreateNext = Create::Person;
		}
		ImGui::SameLine();
		if (ImGui::Button("Create Obstacle"))
		{
			CreateNext = Create::Obstacle;
		}
		ImGui::SameLine();
		if (ImGui::Button("Create Floor Changer"))
		{
			CreateNext = Create::Changer;
		}
		if (ImGui::Button("Clear mouse"))
		{
			CreateNext = Create::None;
		}
	}
	else
	{
		CreateNext = Create::None;
	}

	if (ImGui::TreeNode("Floors"))
	{
		static int index = 1;
		ImGui::InputInt("floor number", &index);
		if (ImGui::Button("Create new floor"))
		{
			m_world.add_floor(index);
		}

		for (auto floor : m_world.get_layout())
		{
			std::stringstream ss;
			ss << "floor " << floor.first << ": " << floor.second.name << " {"
			   << floor.second.group << "}"
			   << "###" << floor.first;
			if (ImGui::TreeNode(ss.str().c_str()))
			{
				if (ImGui::Button("View Floor"))
				{
					viewing_floor_or_group = floor.first;
					m_selection_box = std::nullopt;
				}
				ImGui::SameLine();
				if (ImGui::Button("View Group"))
				{
					viewing_floor_or_group = floor.second.group;
					m_selection_box = std::nullopt;
				}

				static std::string name;
				static std::string group;
				ImGui::InputText("name", &name);
				if (ImGui::Button("submit new name"))
				{
					m_world.set_floor_name(floor.first, name);
				}
				ImGui::InputText("group", &group);
				if (ImGui::Button("submit new group"))
				{
					m_world.set_floor_group(floor.first, group);
				}

				if (ImGui::Button("Delete Floor"))
				{
					m_world.remove_floor(floor.first);
				}
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
	static std::string filename;
	ImGui::InputText("Filename", &filename);
	if (ImGui::Button("Save"))
	{
		SaveToFile(filename);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		LoadFromFile(filename);
	}
	ImGui::Text(
	    "mouse location: %.3f, %.3f",
	    mouse_location.x,
	    mouse_location.y);
	ImGui::End();

	if (m_selection_box)
	{
		if (m_selection_box->size() == 1)
		{
			bool open = true;
			ImGui::Begin("Selection", &open);
			switch (m_selection_box->at(0).index())
			{
			case 0: {
				auto &person_index = std::get<0>(m_selection_box->at(0));
				auto &person = SimRunning
				                   ? m_current_people[person_index]
				                   : m_simulation_start_people[person_index];
				auto to_delete = PersonUI(person);
				if (to_delete)
				{
					open = false;
					m_simulation_start_people.erase(
					    m_simulation_start_people.begin() + person_index);
				}
			}
			break;
			case 1: {
				auto pair = std::get<1>(m_selection_box->at(0));
				ObstacleUI(pair.first, pair.second, open);
			}
			break;
			case 2:
				ChangerUI(std::get<2>(m_selection_box->at(0)).first, open);
				break;
			}
			ImGui::End();
			if (!open)
			{
				m_selection_box = std::nullopt;
			}
		}
	}
}

bool SimManager::PersonUI(Person &person)
{
	if (SimRunning)
	{
		ImGui::Text("State: %s", Person::StateString(person.state));
	}
	else
	{
		if (ImGui::BeginCombo("State", Person::StateString(person.state)))
		{
			if (ImGui::Selectable("susceptible"))
			{
				person.state = Person::susceptible;
			}
			if (ImGui::Selectable("infected"))
			{
				person.state = Person::infected;
			}
			if (ImGui::Selectable("recovered"))
			{
				person.state = Person::recovered;
			}
			ImGui::EndCombo();
		}
	}
	if (ImGui::TreeNode("Routine"))
	{
		std::optional<size_t> to_delete;
		for (size_t i = 0; i < person.routine.actions.size(); i++)
		{
			ImGui::PushID(i);
			std::stringstream ss;
			ss << "Action " << i << "###ActionNode";
			if (ImGui::TreeNode(ss.str().c_str()))
			{
				double minutes = person.routine.actions[i].when / 60;
				ImGui::InputDouble("When", &minutes, 0, 0, "%.3f minutes");
				person.routine.actions[i].when = minutes * 60;
				ImGui::InputInt("Floor", &person.routine.actions[i].where.first);
				glm::vec2 where = person.routine.actions[i].where.second;
				ImGui::InputFloat2("Where", glm::value_ptr(where));
				person.routine.actions[i].where.second = where;

				if (ImGui::Button("delete"))
				{
					to_delete = i;
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
		}
		if (to_delete)
		{
			person.routine.actions.erase(
			    person.routine.actions.begin() + *to_delete);
		}
		if (ImGui::Button("add"))
		{
			person.routine.actions.emplace_back();
		}
		double minutes = person.routine.repeat_interval / 60;
		ImGui::InputDouble("Repeat every", &minutes, 0, 0, "%.6f minutes");
		person.routine.repeat_interval = minutes * 60;
		ImGui::TreePop();
	}
	if (person.state == Person::infected)
	{
		double orig_seconds = person.infection_finish_time - sim_time;
		int seconds = orig_seconds;
		int minutes = seconds / 60;
		int hours = minutes / 60;
		int days = hours / 24;
		hours %= 24;
		minutes %= 60;
		seconds %= 60;
		float shown_seconds = orig_seconds - std::floor(orig_seconds) + seconds;
		if (SimRunning)
		{
			ImGui::Text(
			    "time until infection wears off: %i days %i hours %i minutes %f seconds",
			    days,
			    hours,
			    minutes,
			    shown_seconds);
		}
		else
		{
			int day_temp = days;
			if (ImGui::InputInt(
			        "Days",
			        &day_temp,
			        0,
			        0,
			        ImGuiInputTextFlags_EnterReturnsTrue))
			{
				days = day_temp;
			}
			int hour_temp = hours;
			if (ImGui::InputInt(
			        "Hours",
			        &hour_temp,
			        0,
			        0,
			        ImGuiInputTextFlags_EnterReturnsTrue))
			{
				hours = hour_temp;
			}
			int minute_temp = minutes;
			if (ImGui::InputInt(
			        "Minutes",
			        &minute_temp,
			        0,
			        0,
			        ImGuiInputTextFlags_EnterReturnsTrue))
			{
				minutes = minute_temp;
			}
			float temp = shown_seconds;
			if (ImGui::InputFloat(
			        "Seconds",
			        &temp,
			        0,
			        0,
			        "%.3f",
			        ImGuiInputTextFlags_EnterReturnsTrue))
			{
				shown_seconds = temp;
			}
			person.infection_finish_time
			    = sim_time
			      + (((days * 24 + hours) * 60 + minutes) * 60 + shown_seconds);
		}
	}
	ImGui::Text("Current Floor: %i", person.floor);
	if (!SimRunning)
	{
		static int current_floor = person.floor;
		ImGui::InputInt("Floor", &current_floor);
		if (ImGui::Button("Teleport to floor"))
		{
			person.floor = current_floor;
		}
		if (ImGui::Button("delete"))
		{
			return true;
		}
	}
	return false;
}

void SimManager::ObstacleUI(int floor, size_t index, bool &open)
{
	if (!m_world.get_layout().contains(floor) || SimRunning)
	{
		open = false;
		return;
	}
	auto &obstacle = m_world.get_obstacle(floor, index);
	if (ImGui::Checkbox("blocks movement", &obstacle.blocks_movement))
	{
		m_world.get_layout().at(floor).recalc();
	}
	ImGui::Checkbox("blocks infection", &obstacle.blocks_infection);
	if (ImGui::Button("delete"))
	{
		m_world.remove_obstacle(floor, index);
		open = false;
	}
}

void SimManager::ChangerUI(size_t changer_index, bool &open)
{
	auto &changer = m_world.get_floor_changer(changer_index);
	if (!SimRunning)
	{
		ImGui::InputInt("Floor A", &changer.a.first);
		ImGui::InputInt("Floor B", &changer.b.first);
		if (ImGui::Button("delete"))
		{
			m_world.remove_floor_changer(changer_index);
			open = false;
		}
	}
	else
	{
		ImGui::Text("Floor A: %i", changer.a.first);
		ImGui::Text("Floor B: %i", changer.b.first);
	}
}

void SimManager::Scroll(double amount, glm::dvec2 mouse_pos)
{
	auto direction = glm::sign(amount);
	amount = 1 + (glm::abs(amount) * mousewheel_sensitivity);
	glm::dvec2 scale_change = viewport.zw();
	if (direction == 1)
	{
		viewport.z *= amount;
		viewport.w *= amount;
	}
	else if (direction == -1)
	{
		viewport.z /= amount;
		viewport.w /= amount;
	}
	scale_change -= viewport.zw();
	viewport.x += scale_change.x * mouse_pos.x;
	viewport.y += scale_change.y * mouse_pos.y;
}

bool SimManager::is_visable(int floor) const
{
	const auto &map = m_world.get_layout();
	if (!map.contains(floor))
	{
		return false;
	}
	if (viewing_floor_or_group.index() == 0)
	{
		return floor == std::get<0>(viewing_floor_or_group);
	}
	else
	{
		return map.at(floor).group == std::get<1>(viewing_floor_or_group);
	}
}

void SimManager::Click(glm::dvec2 click, bool ctrl)
{
	if (ctrl)
	{
		if (marked_location)
		{
			marked_location = std::nullopt;
		}
		else
		{
			marked_location = click;
		}
		return;
	}
	switch (CreateNext)
	{
	case Create::None:
		break;
	case Create::Person: {
		m_simulation_start_people.push_back({});
		auto &person
		    = m_simulation_start_people[m_simulation_start_people.size() - 1];
		person.position = click;
		person.floor = std::get<0>(viewing_floor_or_group);
	}
		return;
	case Create::Obstacle:
		m_world.add_obstacle(
		    std::get<0>(viewing_floor_or_group),
		    {click - glm::dvec2{0.02}, glm::dvec2{0.04, 0.04}, 0});
		return;
	case Create::Changer:
		m_world.add_floor_changer(
		    {{std::get<0>(viewing_floor_or_group), click},
		     {std::get<0>(viewing_floor_or_group), click}});
		return;
	case Create::Paste: {
		for (auto thing : *m_paste_buffer)
		{
			if (viewing_floor_or_group.index() == 0)
			{
				auto floor = std::get<0>(viewing_floor_or_group);
				if (thing.index() == 0)
				{
					std::get<0>(thing).position += click;
					std::get<0>(thing).floor = floor;
					m_simulation_start_people.emplace_back(std::get<0>(thing));
				}
				else
				{
					std::get<1>(thing).position += click;
					m_world.add_obstacle(floor, std::get<1>(thing));
				}
			}
		}
		return;
	}
	}

	auto &people = SimRunning ? m_current_people : m_simulation_start_people;
	for (size_t i = 0; i < people.size(); i++)
	{
		if (is_visable(people[i].floor))
		{
			if (glm::distance(click, people[i].position) < 0.01)
			{
				m_selection_box = std::vector{
				    decltype(m_selection_box)::value_type::value_type{
				        std::in_place_index<0>,
				        i}};
				return;
			}
		}
	}

	for (auto &floor : m_world.get_layout())
	{
		if (is_visable(floor.first))
		{
			for (size_t i = 0; i < floor.second.obstacles.size(); i++)
			{
				auto &obstacle = floor.second.obstacles[i];
				auto center = obstacle.position + obstacle.size / 2.0;
				auto rotate_around_obstacle
				    = glm::translate(glm::dmat4{1}, glm::dvec3{center, 0})
				      * glm::rotate(
				          glm::dmat4{1},
				          -obstacle.rotation,
				          glm::dvec3{0, 0, 1})
				      * glm::translate(glm::dmat4{1}, -glm::dvec3{center, 0});
				glm::dvec2 rotated_click
				    = rotate_around_obstacle * glm::dvec4{click, 0, 1};
				auto vertecies = obstacle.get_vertecies(0, false);
				if (is_within(rotated_click.x, vertecies[0].x, vertecies[2].x)
				    && is_within(
				        rotated_click.y,
				        vertecies[0].y,
				        vertecies[2].y))
				{
					m_selection_box = std::vector{
					    decltype(m_selection_box)::value_type::value_type{
					        std::pair(floor.first, i)}};
					return;
				}
			}
		}
	}

	for (size_t i = 0; i < m_world.get_floor_changers().size(); i++)
	{
		auto &changer = m_world.get_floor_changer(i);
		if (is_visable(changer.a.first))
		{
			if (glm::distance(changer.a.second, click) < 0.005)
			{
				m_selection_box = std::vector{
				    decltype(m_selection_box)::value_type::value_type{
				        std::in_place_index<2>,
				        std::pair{i, true}}};
				return;
			}
		}
		if (is_visable(changer.b.first))
		{
			if (glm::distance(changer.b.second, click) < 0.01)
			{
				m_selection_box = std::vector{
				    decltype(m_selection_box)::value_type::value_type{
				        std::in_place_index<2>,
				        std::pair{i, false}}};
				return;
			}
		}
	}

	m_selection_box = std::nullopt;
}

void SimManager::StartDrag(glm::dvec2 where, bool ctrl)
{
	last_drag = where;

	if (CreateNext == Create::Obstacle && viewing_floor_or_group.index() == 0)
	{
		DragState = CreateObstacle;
		return;
	}

	if (m_selection_box)
	{
		if (ctrl)
		{
			DragState = DragSelection;
			return;
		}
		if (m_selection_box->size() == 1)
		{
			if (m_selection_box->at(0).index() == 1)
			{
				auto obstacle_index = std::get<1>(m_selection_box->at(0));
				if (m_world.get_layout().contains(obstacle_index.first))
				{
					auto &obstacle = m_world.get_obstacle(
					    obstacle_index.first,
					    obstacle_index.second);
					auto center = obstacle.position + obstacle.size / 2.0;
					auto inverse_rotation
					    = glm::translate(glm::dmat4{1}, glm::dvec3{center, 0})
					      * glm::rotate(
					          glm::dmat4{1},
					          -obstacle.rotation,
					          glm::dvec3{0, 0, 1})
					      * glm::translate(
					          glm::dmat4{1},
					          -glm::dvec3{center, 0});
					glm::dvec2 rotated_click
					    = inverse_rotation * glm::dvec4{where, 0, 1};
					auto main_vertecies = obstacle.get_vertecies(0.04, false);
					for (size_t i = 0; i < main_vertecies.size(); i += 2)
					{
						auto next = i + 1;
						if (next == main_vertecies.size())
						{
							next = 0;
						}
						auto insert_point = (main_vertecies.begin() + i) + 1;
						main_vertecies.insert(
						    insert_point,
						    glm::mix(
						        main_vertecies[i],
						        main_vertecies[next],
						        0.5));
					}
					for (size_t i = 0; i < main_vertecies.size(); i++)
					{
						auto vertex = main_vertecies[i];
						if (is_within(
						        rotated_click.x,
						        vertex.x - 0.005,
						        vertex.x + 0.005)
						    && is_within(
						        rotated_click.y,
						        vertex.y - 0.005,
						        vertex.y + 0.005))
						{
							DragState = ScaleObstacle;
							std::array<int, 2> or_together;
							if ((i & 1) != 1)
							{
								int down = i - 1, up = i + 1;
								if (down == -1)
								{
									down = 7;
								}
								or_together[0] = down;
								or_together[1] = up;
							}
							else
							{
								or_together[0] = i;
								or_together[1] = i;
							}
							for (auto todo : or_together)
							{
								if (todo == 1)
								{
									ScaleCorner_Top = 1;
								}
								if (todo == 3)
								{
									ScaleCorner_Right = 1;
								}
								if (todo == 5)
								{
									ScaleCorner_Bottom = 1;
								}
								if (todo == 7)
								{
									ScaleCorner_Left = 1;
								}
							}
							return;
						}
					}
					main_vertecies = obstacle.get_vertecies(0.08, false);
					if (is_within(
					        rotated_click.x,
					        main_vertecies[0].x,
					        main_vertecies[2].x)
					    && is_within(
					        rotated_click.y,
					        main_vertecies[0].y,
					        main_vertecies[2].y))
					{
						DragState = RotatingObstacle;
						return;
					}
				}
			}
		}
	}
	if (ctrl && viewing_floor_or_group.index() == 0 && !SimRunning)
	{
		DragState = Select;
		return;
	}

	//default
	DragState = Screen;
}

void SimManager::UpdateDrag(glm::dvec2 where)
{
	if (SimRunning && DragState != Screen)
	{
		return;
	}
	switch (DragState)
	{
	case None:
		break;
	case Screen:
		viewport.x -= last_drag.x - where.x;
		viewport.y -= last_drag.y - where.y;
		break;
	case DragSelection: {
		for (auto &thing : *m_selection_box)
		{
			if (thing.index() == 0)
			{
				auto &person = m_simulation_start_people[std::get<0>(thing)];
				person.position -= last_drag - where;
			}
			else if (thing.index() == 1)
			{
				auto obstacle_index = std::get<1>(thing);
				auto &obstacle = m_world.get_obstacle(
				    obstacle_index.first,
				    obstacle_index.second);
				obstacle.position -= last_drag - where;
			}
			else
			{
				auto &changer
				    = m_world.get_floor_changer(std::get<2>(thing).first);
				auto selected_a_or_b = std::get<2>(thing).second;
				if (selected_a_or_b)
				{
					changer.a.second -= last_drag - where;
				}
				else
				{
					changer.b.second -= last_drag - where;
				}
			}
		}
	}
	break;
	case RotatingObstacle: {
		auto obstacle_index = std::get<1>(m_selection_box->at(0));
		auto &obstacle
		    = m_world.get_obstacle(obstacle_index.first, obstacle_index.second);
		auto center = obstacle.position + obstacle.size / 2.0;
		auto last_drag_rel = last_drag - center;
		auto where_rel = where - center;
		obstacle.rotation -= std::atan2(last_drag_rel.y, last_drag_rel.x)
		                     - std::atan2(where_rel.y, where_rel.x);
	}
	break;
	case CreateObstacle:
		break;
	case Select:
		break;
	case ScaleObstacle: {
		auto obstacle_index = std::get<1>(m_selection_box->at(0));
		if (!m_world.get_layout().contains(obstacle_index.first))
		{
			break;
		}
		auto &obstacle
		    = m_world.get_obstacle(obstacle_index.first, obstacle_index.second);
		auto center = obstacle.position + obstacle.size / 2.0;
		auto inverse_rotation
		    = glm::translate(glm::dmat4{1}, glm::dvec3{center, 0})
		      * glm::rotate(
		          glm::dmat4{1},
		          -obstacle.rotation,
		          glm::dvec3{0, 0, 1})
		      * glm::translate(glm::dmat4{1}, -glm::dvec3{center, 0});
		glm::dvec2 diff = (inverse_rotation * glm::dvec4{where, 0, 1})
		                  - (inverse_rotation * glm::dvec4{last_drag, 0, 1});

		if (ScaleCorner_Left)
		{
			if ((obstacle.size.x - diff.x) >= 0.01)
			{
				obstacle.position.x += diff.x;
				obstacle.size.x -= diff.x;
			}
		}
		if (ScaleCorner_Right)
		{
			if ((obstacle.size.x + diff.x) >= 0.01)
			{
				obstacle.size.x += diff.x;
			}
		}
		if (ScaleCorner_Top)
		{
			if ((obstacle.size.y - diff.y) >= 0.01)
			{
				obstacle.position.y += diff.y;
				obstacle.size.y -= diff.y;
			}
		}
		if (ScaleCorner_Bottom)
		{
			if ((obstacle.size.y + diff.y) >= 0.01)
			{
				obstacle.size.y += diff.y;
			}
		}
	}
	break;
	}
	if (DragState != Screen && DragState != CreateObstacle
	    && DragState != Select)
	{
		last_drag = where;
	}
}

void SimManager::StopDrag(glm::dvec2 where)
{
	if (SimRunning)
	{
		return;
	}
	UpdateDrag(where);
	if (DragState == CreateObstacle)
	{
		glm::dvec2 min, max;
		min.x = glm::min(last_drag.x, where.x);
		min.y = glm::min(last_drag.y, where.y);
		max.x = glm::max(last_drag.x, where.x);
		max.y = glm::max(last_drag.y, where.y);
		m_world.add_obstacle(
		    std::get<0>(viewing_floor_or_group),
		    {min, max - min, 0});
	}
	else if (DragState == Select)
	{
		auto floor = std::get<0>(viewing_floor_or_group);
		if (m_world.get_layout().contains(floor))
		{
			glm::dvec2 min, max;
			min.x = glm::min(last_drag.x, where.x);
			min.y = glm::min(last_drag.y, where.y);
			max.x = glm::max(last_drag.x, where.x);
			max.y = glm::max(last_drag.y, where.y);
			Obstacle selection{min, max - min, 0};
			m_selection_box.emplace();
			for (size_t i = 0; i < m_simulation_start_people.size(); i++)
			{
				auto &person = m_simulation_start_people[i];
				if (person.floor == floor)
				{
					if (selection.intersects(person.position))
					{
						m_selection_box->emplace_back(std::in_place_index<0>, i);
					}
				}
			}
			for (size_t i = 0; i < m_world.get_floor_changers().size(); i++)
			{
				auto &changer = m_world.get_floor_changer(i);
				bool added = false;
				if (changer.a.first == floor)
				{
					if (selection.intersects(changer.a.second))
					{
						m_selection_box->emplace_back(
						    std::in_place_index<2>,
						    std::pair{i, true});
						added = true;
					}
				}
				if (changer.b.first == floor && !added)
				{
					if (selection.intersects(changer.b.second))
					{
						m_selection_box->emplace_back(
						    std::in_place_index<2>,
						    std::pair{i, false});
					}
				}
			}
			for (size_t i = 0;
			     i < m_world.get_layout().at(floor).obstacles.size();
			     ++i)
			{
				auto &obstacle = m_world.get_layout().at(floor).obstacles.at(i);
				if (selection.intersects(obstacle))
				{
					m_selection_box->emplace_back(
					    std::in_place_index<1>,
					    std::pair{floor, i});
				}
			}
			if (m_selection_box->size() == 0)
			{
				m_selection_box = std::nullopt;
			}
		}
	}
	DragState = None;
	ScaleCorner_Top = 0;
	ScaleCorner_Right = 0;
	ScaleCorner_Bottom = 0;
	ScaleCorner_Left = 0;
}

void SimManager::LoadFromFile(std::string filename)
{
	std::ifstream file{filename};
	boost::archive::text_iarchive ar{file};
	ar >> m_world;
	ar >> m_simulation_start_people;
}

void SimManager::SaveToFile(std::string filename)
{
	std::ofstream file{filename};
	boost::archive::text_oarchive ar{file};
	ar << m_world;
	ar << m_simulation_start_people;
}

void SimManager::KeyClick(SDL_Scancode key, bool ctrl)
{
	switch (key)
	{
	case SDL_SCANCODE_Q:
		m_selection_box = std::nullopt;
		CreateNext = Create::None;
		if (DragState == CreateObstacle)
		{
			DragState = None;
		}
		break;

	case SDL_SCANCODE_DELETE:
		if (!SimRunning)
		{
			if (m_selection_box)
			{
				for (auto iter = m_selection_box->rbegin();
				     iter != m_selection_box->rend();
				     iter++)
				{
					auto &thing = *iter;
					if (thing.index() == 0)
					{
						m_simulation_start_people.erase(
						    m_simulation_start_people.begin()
						    + std::get<0>(thing));
					}
					else if (thing.index() == 1)
					{
						auto index = std::get<1>(thing);
						m_world.remove_obstacle(index.first, index.second);
					}
					else
					{
						auto index = std::get<2>(thing);
						m_world.remove_floor_changer(index.first);
					}
				}
				m_selection_box = std::nullopt;
			}
		}
		break;

	case SDL_SCANCODE_C:
		if (ctrl && !SimRunning && m_selection_box)
		{
			CreateNext = Create::None;
			auto floor = std::get<0>(viewing_floor_or_group);
			if (!m_world.get_layout().contains(floor))
			{
				break;
			}
			m_paste_buffer.emplace();
			glm::dvec2 min_pos{std::numeric_limits<double>::max()}, max_pos;
			for (auto &thing : *m_selection_box)
			{
				if (thing.index() == 0)
				{
					auto &person
					    = m_simulation_start_people.at(std::get<0>(thing));
					m_paste_buffer->emplace_back(person);
					min_pos.x = glm::min(min_pos.x, person.position.x);
					min_pos.y = glm::min(min_pos.y, person.position.y);
					max_pos.x = glm::max(max_pos.x, person.position.x);
					max_pos.y = glm::max(max_pos.y, person.position.y);
				}
				else if (thing.index() == 1)
				{
					auto obstacle_index = std::get<1>(thing);
					auto &obstacle = m_world.get_obstacle(
					    obstacle_index.first,
					    obstacle_index.second);
					m_paste_buffer->emplace_back(obstacle);
					min_pos.x = glm::min(min_pos.x, obstacle.position.x);
					min_pos.y = glm::min(min_pos.y, obstacle.position.y);
					max_pos.x = glm::max(
					    max_pos.x,
					    obstacle.position.x + obstacle.size.x);
					max_pos.y = glm::max(
					    max_pos.y,
					    obstacle.position.y + obstacle.size.y);
				}
			}
			if (m_paste_buffer->size() == 0)
			{
				m_paste_buffer = std::nullopt;
				break;
			}
			auto center = glm::mix(min_pos, max_pos, 0.5);
			for (auto &thing : *m_paste_buffer)
			{
				if (thing.index() == 0)
				{
					std::get<0>(thing).position -= center;
				}
				else
				{
					std::get<1>(thing).position -= center;
				}
			}
		}
		break;

	case SDL_SCANCODE_V:
		if (ctrl && !SimRunning && m_paste_buffer)
		{
			auto floor = std::get<0>(viewing_floor_or_group);
			if (!m_world.get_layout().contains(floor))
			{
				break;
			}
			CreateNext = Create::Paste;
		}
		break;

	default:
		break;
	}
}
