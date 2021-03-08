#pragma once

#include <functional>
#include <optional>
#include <random>

#include <glm/ext.hpp>

#include "person.hpp"
#include "world.hpp"

class SimManager {

	public:
	SimManager(std::function<double(std::optional<double>)> timescale);

	void SimStep(double dt);

	void MoveStep(double dt);
	void InfectStep(double dt);
	
	void StartDrag(glm::dvec2);
	void UpdateDrag(glm::dvec2);
	void StopDrag(glm::dvec2);
	void Click(glm::dvec2);

	void LoadFromFile(std::string filename);
	void SaveToFile(std::string filename);

	void DrawUI();

	private:
	World m_world;
	std::vector<Person> m_current_people;
	std::vector<Person> m_simulation_start_people;
	bool SimRunning = false;
	std::function<double(std::optional<double>)> m_timescale;
	double sim_time = 0;
	std::mt19937_64 rng{1337};

	//chance of infection: 1/(1 + distance * inf_dist_mult) * infection_chance * dt
	double minimum_infection_range = 0.05;
	double infection_chance = 0.5;
	double infection_distance_multiplier = 0.05;
	double min_infection_duration{4.0};
	double max_infection_duration{8.0};

	friend class Renderer;
};
