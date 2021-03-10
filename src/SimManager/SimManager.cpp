#include "SimManager.hpp"

#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <sstream>

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
	m_current_selection.emplace(
	    std::in_place_index<1>,
	    std::pair<int, size_t>{1, 0});
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
			if (person.Routine)
			{
				person.going_along = m_world.calculate_path(
				    person.floor,
				    person.position,
				    person.Routine->first,
				    person.Routine->second);
				person.going_to = 0;
				person.Routine = std::nullopt;
			}
			return;
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
			    || second_person.state != Person::susceptible)
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
				}
				ImGui::SameLine();
				if (ImGui::Button("View Group"))
				{
					viewing_floor_or_group = floor.second.group;
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
	ImGui::Text(
	    "mouse location: %.3f, %.3f",
	    mouse_location.x,
	    mouse_location.y);
	ImGui::End();

	if (m_current_selection)
	{
		bool open = true;
		ImGui::Begin("Selection", &open);
		switch (m_current_selection->index())
		{
		case 0: {
			auto &person_index = std::get<0>(*m_current_selection);
			auto &person = SimRunning ? m_current_people[person_index]
			                          : m_simulation_start_people[person_index];
			PersonUI(person);
		}
		break;
		case 1: {
			auto pair = std::get<1>(*m_current_selection);
			ObstacleUI(pair.first, pair.second, open);
		}
		break;
		case 2:
			ChangerUI(std::get<2>(*m_current_selection));
			break;
		}
		ImGui::End();
		if (!open)
		{
			m_current_selection = std::nullopt;
		}
	}
}

void SimManager::PersonUI(Person &person)
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
	if (person.Routine)
	{
		ImGui::Text("Going to go to %i {%f, %f}", person.Routine->first, person.Routine->second.x, person.Routine->second.y);
	}
	static int floor;
	static glm::vec2 pos;
	ImGui::InputInt("Target Floor", &floor);
	ImGui::InputFloat2("Target Location", glm::value_ptr(pos));
	if(ImGui::Button("Submit"))
	{
		person.Routine = {floor, glm::dvec2{pos}};
	}
	ImGui::SameLine();
	if(ImGui::Button("Delete"))
	{
		person.Routine = std::nullopt;
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
	}
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

void SimManager::ChangerUI(size_t changer_index)
{
	auto &changer = m_world.get_floor_changer(changer_index);
	ImGui::InputInt("Floor A", &changer.a.first);
	ImGui::InputInt("Floor B", &changer.b.first);
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
	}

	auto &people = SimRunning ? m_current_people : m_simulation_start_people;
	for (size_t i = 0; i < people.size(); i++)
	{
		if (is_visable(people[i].floor))
		{
			if (glm::distance(click, people[i].position) < 0.01)
			{
				m_current_selection = decltype(
				    m_current_selection)::value_type{std::in_place_index<0>, i};
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
					m_current_selection = std::pair(floor.first, i);
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
				selected_a_or_b = true;
				m_current_selection = decltype(
				    m_current_selection)::value_type{std::in_place_index<2>, i};
				return;
			}
		}
		if (is_visable(changer.b.first))
		{
			if (glm::distance(changer.b.second, click) < 0.01)
			{
				selected_a_or_b = false;
				m_current_selection = decltype(
				    m_current_selection)::value_type{std::in_place_index<2>, i};
				return;
			}
		}
	}

	m_current_selection = std::nullopt;
}

void SimManager::StartDrag(glm::dvec2 where)
{
	std::cout << glm::to_string(where) << '\n';
	last_drag = where;

	if (m_current_selection)
	{
		if (m_current_selection->index() == 0)
		{
			auto &people
			    = SimRunning ? m_current_people : m_simulation_start_people;
			if (glm::distance(
			        where,
			        people[std::get<0>(*m_current_selection)].position)
			    < 0.01)
			{
				DragState = DragPerson;
				return;
			}
		}
		else if (m_current_selection->index() == 1)
		{
			auto obstacle_index = std::get<1>(*m_current_selection);
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
				      * glm::translate(glm::dmat4{1}, -glm::dvec3{center, 0});
				glm::dvec2 rotated_click
				    = inverse_rotation * glm::dvec4{where, 0, 1};
				auto main_vertecies = obstacle.get_vertecies(0, false);
				for (auto &a : main_vertecies)
				{
					std::cout << glm::to_string(a) << " ";
				}
				std::cout << "\n";
				if (is_within(
				        rotated_click.x,
				        main_vertecies[0].x,
				        main_vertecies[2].x)
				    && is_within(
				        rotated_click.y,
				        main_vertecies[0].y,
				        main_vertecies[2].y))
				{
					DragState = MovingObstacle;
					return;
				}
				main_vertecies = obstacle.get_vertecies(0.08, false);
				for (auto &a : main_vertecies)
				{
					std::cout << glm::to_string(a) << " ";
				}
				std::cout << "\n";
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
		else
		{
			glm::dvec2 position;
			if (selected_a_or_b)
			{
				position
				    = m_world
				          .get_floor_changer(std::get<2>(*m_current_selection))
				          .a.second;
			}
			else
			{
				position
				    = m_world
				          .get_floor_changer(std::get<2>(*m_current_selection))
				          .b.second;
			}
			if (glm::distance(where, position) < 0.01)
			{
				DragState = DragFloorChanger;
				return;
			}
		}
	}

	//default
	DragState = Screen;
}

void SimManager::UpdateDrag(glm::dvec2 where)
{
	std::cout << glm::to_string(where) << '\n';
	if (SimRunning)
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
	case DragPerson: {
		auto &person
		    = m_simulation_start_people[std::get<0>(*m_current_selection)];
		person.position -= last_drag - where;
	}
	break;
	case MovingObstacle: {
		auto obstacle_index = std::get<1>(*m_current_selection);
		auto &obstacle
		    = m_world.get_obstacle(obstacle_index.first, obstacle_index.second);
		obstacle.position -= last_drag - where;
	}
	break;
	case RotatingObstacle: {
		auto obstacle_index = std::get<1>(*m_current_selection);
		auto &obstacle
		    = m_world.get_obstacle(obstacle_index.first, obstacle_index.second);
		auto center = obstacle.position + obstacle.size / 2.0;
		auto last_drag_rel = last_drag - center;
		auto where_rel = where - center;
		obstacle.rotation -= std::atan2(last_drag_rel.y, last_drag_rel.x)
		                     - std::atan2(where_rel.y, where_rel.x);
	}
	break;
	case DragFloorChanger: {
		auto &changer
		    = m_world.get_floor_changer(std::get<2>(*m_current_selection));
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
	if (DragState != Screen)
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
	DragState = None;
}
