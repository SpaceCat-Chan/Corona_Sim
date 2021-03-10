#pragma once

#include <functional>
#include <optional>
#include <random>

#include <glm/ext.hpp>

#include "SDL.h"

#include "person.hpp"
#include "world.hpp"

class SimManager
{

	public:
	SimManager(std::function<double(std::optional<double>)> timescale);

	void SimStep(double dt);

	void MoveStep(double dt);
	void InfectStep(double dt);

	void StartDrag(glm::dvec2, bool);
	void UpdateDrag(glm::dvec2);
	void StopDrag(glm::dvec2);
	void Click(glm::dvec2, bool);
	void Scroll(double, glm::dvec2);

	void KeyClick(SDL_Scancode key, bool ctrl);

	void LoadFromFile(std::string filename);
	void SaveToFile(std::string filename);

	void DrawUI(glm::dvec2 mouse_location);

	bool is_visable(int floor) const;

	private:
	bool PersonUI(Person &);
	void ObstacleUI(int, size_t, bool &);
	void ChangerUI(size_t, bool &);
	World m_world;
	std::vector<Person> m_current_people;
	std::vector<Person> m_simulation_start_people;
	bool SimRunning = false;
	std::function<double(std::optional<double>)> m_timescale;
	double sim_time = 0;
	std::mt19937_64 rng{1337};
	glm::dvec4 viewport{0, 0, 1, 1};

	std::variant<int, std::string> viewing_floor_or_group{
	    std::in_place_index<0>,
	    1};

	double mousewheel_sensitivity = 0.1;

	std::optional<std::vector<
	    std::variant<size_t, std::pair<int, size_t>, std::pair<size_t, bool>>>>
	    m_selection_box;

	std::optional<std::vector<std::variant<Person, Obstacle>>> m_paste_buffer;

	std::optional<glm::dvec2> marked_location;

	enum
	{
		None,
		Screen,
		DragSelection,
		RotatingObstacle,
		ScaleObstacle,
		CreateObstacle,
		Select
	} DragState;
	uint32_t ScaleCorner_Top : 1 = 0;
	uint32_t ScaleCorner_Right : 1 = 0;
	uint32_t ScaleCorner_Bottom : 1 = 0;
	uint32_t ScaleCorner_Left : 1 = 0;

	enum class Create
	{
		None,
		Person,
		Obstacle,
		Changer,
		Paste
	} CreateNext;

	glm::dvec2 create_obstacle_start;
	glm::dvec2 last_drag;

	//chance of infection: 1/(1 + distance * inf_dist_mult) * infection_chance * dt
	double minimum_infection_range = 0.05;
	double infection_chance = 0.5;
	double infection_distance_multiplier = 0.05;
	double min_infection_duration{4.0};
	double max_infection_duration{8.0};

	friend class Renderer;
};
