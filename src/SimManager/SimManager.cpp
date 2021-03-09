#include "SimManager.hpp"

#include "imgui/imgui.h"

SimManager::SimManager(std::function<double(std::optional<double>)> timescale)
{
	m_timescale = timescale;

	m_world.add_obstacle(1, {{0.1, 0.4}, {0.8, 0.2}, 0.0});

	for (int i = 0; i < 1; i++)
	{
		m_simulation_start_people.emplace_back();
		m_simulation_start_people[i].state = Person::susceptible;
		m_simulation_start_people[i].position = {0.5, 0.3};
		m_simulation_start_people[i].going_along = m_world.calculate_path(
		    1,
		    m_simulation_start_people[i].position,
		    1,
		    {0.5, 0.7});
		m_simulation_start_people[i].going_to = 0;
		m_simulation_start_people[i].floor = 0;
	}
	m_current_selection.emplace(std::in_place_type<size_t>, 0);
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
		if (person.going_to == person.going_along.waypoints.size())
		{
			continue;
		}

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
void SimManager::DrawUI()
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
	}
	if (ImGui::Button("Stop Simulation"))
	{
		SimRunning = false;
	}
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
			break;
		}
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
			int  minute_temp = minutes;
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
}

void SimManager::Scroll(double amount, glm::dvec2 orig_mouse)
{
	auto direction = glm::sign(amount);
	amount = 1 + (glm::abs(amount) * mousewheel_sensitivity);
	glm::dvec2 scale_change = viewport.zw();
	glm::dvec2 mouse_pos = (orig_mouse - viewport.xy()) / viewport.zw();
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

void SimManager::Click(glm::dvec2)
{
	//not yet...
}

void SimManager::StartDrag(glm::dvec2) {}