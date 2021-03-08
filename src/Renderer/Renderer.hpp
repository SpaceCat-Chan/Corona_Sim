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
	uint32_t window_x, window_y;

	Shader basic_shader;

	public:
	bool Init(uint32_t window_x_, uint32_t window_y_, const char *window_name);
	void Draw(const SimManager &manager);
	void Cleanup();
};
