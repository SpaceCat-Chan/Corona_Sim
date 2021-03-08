#include "Renderer.hpp"

#include <iostream>
#include <numbers>

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

	if (auto error = glewInit(); error != GLEW_OK)
	{
		std::cout << "GLEW failed to init: " << glewGetErrorString(error)
		          << std::endl;
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(window);
	ImGui_ImplOpenGL3_Init();

	basic_shader.AddShaderFile("res/basic_shader.vert", GL_VERTEX_SHADER);
	basic_shader.AddShaderFile("res/basic_shader.frag", GL_FRAGMENT_SHADER);

	std::vector<glm::vec2> circle_pos;
	constexpr size_t circle_vertecies = 2000;
	circle_pos.reserve(circle_vertecies);
	constexpr double delta_phi = std::numbers::pi * 2 / circle_vertecies;
	for (size_t i = 0; i < circle_vertecies; i++)
	{
		circle_pos.emplace_back(
		    glm::cos(delta_phi * i),
		    glm::sin(delta_phi * i));
	}
	glCreateVertexArrays(1, &circle_vao);
	glBindVertexArray(circle_vao);

	glCreateBuffers(1, &circle_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*circle_vertecies, circle_pos.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	std::vector<glm::vec2> square_pos = {
		{-0.5, -0.5},
		{0.5, -0.5},
		{0.5, 0.5},
		{-0.5, 0.5}
	};
	glCreateVertexArrays(1, &square_vao);
	glBindVertexArray(square_vao);

	glCreateBuffers(1, &square_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, square_vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*square_pos.size(), square_pos.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return true;
}

void Renderer::StartImGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
}

void Renderer::Draw(const SimManager &manager)
{
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	for (auto obstacle : manager.m_world.get_layout().at(1).obstacles)
	{
		basic_shader.SetUniform("u_model", glm::translate(glm::rotate(glm::scale(glm::dmat3{1.0}, obstacle.size), obstacle.rotation), obstacle.position + obstacle.size/2.0));
		glBindVertexArray(square_vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	for (auto &person : manager.SimRunning ? manager.m_current_people
	                                       : manager.m_simulation_start_people)
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

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_RenderPresent(renderer);
}

void Renderer::Cleanup()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	window.Destroy();
}
