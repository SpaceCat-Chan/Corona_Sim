#include "SimManager.hpp"

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

	SimRunning = true;
	m_current_people = m_simulation_start_people;
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
	//no ui yet
}
