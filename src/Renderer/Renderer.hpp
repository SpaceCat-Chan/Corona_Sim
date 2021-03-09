#pragma once

#include <cstdint>
#include <vector>

#include <SDL.h>

#include "SimManager/SimManager.hpp"
#include "person.hpp"
#include "world.hpp"

#include "Shader.hpp"
#include "Window.hpp"

class Renderer
{
	Window window;
	glm::dvec2 window_size;

	Shader basic_shader{Shader::DeferConstruction{}};

	GLuint circle_vbo;
	GLuint circle_vao;
	GLuint square_vbo;
	GLuint square_vao;

	void draw_sphere(glm::dvec2 point, double radius, glm::dvec4 color);

	public:
	bool Init(uint32_t window_x_, uint32_t window_y_, const char *window_name);
	void StartImGuiFrame();
	void Draw(const SimManager &manager);
	void Cleanup();

	glm::dvec2 ScreenToWorld(glm::dvec2);
	glm::dvec2 WorldToScreen(glm::dvec2);
	glm::dvec2 ScreenToViewport(glm::dvec2, const SimManager&);
	glm::dvec2 ViewportToScreen(glm::dvec2, const SimManager&);
	glm::dvec2 WorldToViewport(glm::dvec2, const SimManager&);
	glm::dvec2 ViewportToWorld(glm::dvec2, const SimManager&);
};
