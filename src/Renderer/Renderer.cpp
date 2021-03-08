#include "Renderer.hpp"

#include <iostream>

#include <SDL2_gfxPrimitives.h>

bool Renderer::Init(
    uint32_t window_x_,
    uint32_t window_y_,
    const char *window_name)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(
	    SDL_GL_CONTEXT_PROFILE_MASK,
	    SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetSwapInterval(1);

	window_x = window_x_;
	window_y = window_y_;
	window.Load(window_name, window_x_, window_y_);
	window.Bind();

	if(auto error = glewInit(); error != GLEW_OK)
	{
		std::cout << "GLEW failed to init: " << glewGetErrorString(error) << std::endl;
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(window);
	ImGui_ImplOpenGL3_Init();

	basic_shader.AddShaderFile("res/basic_shader.vert", GL_VERTEX_SHADER);
	basic_shader.AddShaderFile("res/basic_shader.frag", GL_FRAGMENT_SHADER);

	return true;
}

void Renderer::Draw(const SimManager &manager)
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BlendMode::SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 127, 127, 127, 255);
	for (auto obstacle : manager.m_world.get_layout().at(1).obstacles)
	{
		SDL_FRect rect;
		rect.x = obstacle.position.x * window_x;
		rect.y = obstacle.position.y * window_y;
		rect.w = obstacle.size.x * window_x;
		rect.h = obstacle.size.y * window_y;
		SDL_RenderFillRectF(renderer, &rect);
	}

	for (auto &person : manager.SimRunning ? manager.m_current_people : manager.m_simulation_start_people)
	{
		switch (person.state)
		{
		case Person::susceptible:
			filledCircleRGBA(
			    renderer,
			    person.position.x * window_x,
			    person.position.y * window_y,
			    10,
			    0,
			    255,
			    0,
			    255);
			break;

		case Person::infected:
			filledCircleRGBA(
			    renderer,
			    person.position.x * window_x,
			    person.position.y * window_y,
			    10,
			    255,
			    0,
			    0,
			    255);
			break;
		case Person::recovered:
			filledCircleRGBA(
			    renderer,
			    person.position.x * window_x,
			    person.position.y * window_y,
			    10,
			    174,
			    174,
			    174,
			    255);
			break;
		}
	}
	SDL_RenderPresent(renderer);
}

void Renderer::Cleanup()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}
