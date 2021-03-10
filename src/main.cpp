#include <chrono>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

#include <SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "Renderer/Renderer.hpp"
#include "SimManager/SimManager.hpp"
#include "person.hpp"
#include "world.hpp"

int main()
{
	double timescale = 1;
	SimManager manager{[&timescale](std::optional<double> desired) {
		if (desired)
		{
			timescale = *desired;
		}
		return timescale;
	}};

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "unable to init SDL: " << SDL_GetError() << std::endl;
		return -1;
	}
	constexpr int window_x = 1000, window_y = 1000;
	Renderer renderer;
	renderer.Init(window_x, window_y, "Simulation");

	ImGuiIO &io = ImGui::GetIO();

	auto last_frame = std::chrono::steady_clock::now();

	bool quit = false;
	double total_time = 0;
	constexpr double time_per_step = 1.0 / 60.0;
	bool dragging = false;
	bool mouse_down = false;
	glm::dvec2 mouse_click_position;
	while (!quit)
	{
		auto this_frame = std::chrono::steady_clock::now();
		auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(
		    this_frame - last_frame);
		last_frame = this_frame;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_MOUSEWHEEL:
				if (!io.WantCaptureMouse)
				{
					if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
					{
						event.wheel.y *= -1;
					}
					manager.Scroll(
					    event.wheel.y,
					    renderer.ScreenToWorld(io.MousePos, manager));
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (!io.WantCaptureMouse)
				{
					mouse_click_position
					    = renderer.ScreenToWorld(io.MousePos, manager);
					mouse_down = true;
				}
				break;
			case SDL_MOUSEMOTION:
				if (!io.WantCaptureMouse && mouse_down)
				{
					if (!dragging)
					{
						if (glm::distance(
						        renderer.WorldToScreen(
						            mouse_click_position,
						            manager),
						        glm::dvec2{io.MousePos})
						    > 20.0)
						{
							dragging = true;
							manager.StartDrag(mouse_click_position, io.KeyCtrl);
						}
					}
					else
					{
						manager.UpdateDrag(
						    renderer.ScreenToWorld(io.MousePos, manager));
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (!io.WantCaptureMouse)
				{
					if (dragging)
					{
						dragging = false;
						manager.StopDrag(
						    renderer.ScreenToWorld(io.MousePos, manager));
					}
					else
					{
						manager.Click(
						    renderer.ScreenToWorld(io.MousePos, manager), io.KeyCtrl);
					}
					mouse_down = false;
				}
				break;
			case SDL_KEYUP:
				if(!io.WantCaptureKeyboard)
				{
					manager.KeyClick(event.key.keysym.scancode, io.KeyCtrl);
				}
				break;
			}
		}

		total_time += dt.count() * timescale;
		while (total_time > time_per_step)
		{
			manager.SimStep(time_per_step);
			total_time -= time_per_step;
		}

		renderer.StartImGuiFrame();
		manager.DrawUI(renderer.ScreenToWorld(io.MousePos, manager));
		renderer.Draw(manager, renderer.ScreenToWorld(io.MousePos, manager));
	}
	renderer.Cleanup();
	SDL_Quit();
}
