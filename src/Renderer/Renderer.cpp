#include "Renderer.hpp"

#include <iostream>
#include <numbers>

#include <glm/gtx/string_cast.hpp>

#include <SDL2_gfxPrimitives.h>

void static GLAPIENTRY MessageCallback(
    GLenum,
    GLenum type,
    GLuint,
    GLenum severity,
    GLsizei,
    const GLchar *message,
    const void *)
{
	// SDL_SetRelativeMouseMode(SDL_TRUE);
	if (type == 0x8251 || type == 0x8250)
	{
		return;
	}
	std::cerr << "GL CALLBACK: "
	          << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "")
	          << " type = " << std::hex << type << ", severity = " << severity
	          << ", message = " << message << '\n';
}

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

	window_size = glm::dvec2{window_x_, window_y_};
	window.Load(window_name, window_x_, window_y_);
	window.Bind();

	if (auto error = glewInit(); error != GLEW_OK)
	{
		std::cout << "GLEW failed to init: " << glewGetErrorString(error)
		          << std::endl;
		return false;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, nullptr);

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
	constexpr double delta_phi = std::numbers::pi * 2.0 / circle_vertecies;
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

	glBufferData(
	    GL_ARRAY_BUFFER,
	    sizeof(glm::vec2) * circle_vertecies,
	    circle_pos.data(),
	    GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

	std::vector<glm::vec2> square_pos
	    = {{-0.5, -0.5}, {0.5, -0.5}, {0.5, 0.5}, {-0.5, 0.5}};
	glCreateVertexArrays(1, &square_vao);
	glBindVertexArray(square_vao);

	glCreateBuffers(1, &square_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, square_vbo);

	glBufferData(
	    GL_ARRAY_BUFFER,
	    sizeof(glm::vec2) * square_pos.size(),
	    square_pos.data(),
	    GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
	/*
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	*/
	glDisable(GL_CULL_FACE);

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
	glm::dmat4 view_proj
	    = glm::ortho(0.0, 1.0, 1.0, 0.0)
	      * glm::translate(glm::dmat4{1}, glm::dvec3{manager.viewport.xy(), 0})
	      * glm::scale(glm::dmat4{1}, glm::dvec3{manager.viewport.zw(), 1});
	basic_shader.SetUniform("u_view_proj", view_proj);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	for (auto obstacle : manager.m_world.get_layout().at(1).obstacles)
	{
		basic_shader.SetUniform(
		    "u_model",
		    glm::rotate(
		        glm::scale(
		            glm::translate(
		                glm::dmat4{1},
		                glm::dvec3{obstacle.position + obstacle.size / 2.0, 0}),
		            glm::dvec3{obstacle.size, 1}),
		        obstacle.rotation,
		        glm::dvec3{0, 0, 1}));
		basic_shader.SetUniform("u_color", glm::dvec4{0.5, 0.5, 0.5, 1.0});
		glBindVertexArray(square_vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	for (auto &person : manager.SimRunning ? manager.m_current_people
	                                       : manager.m_simulation_start_people)
	{
		switch (person.state)
		{
		case Person::susceptible:
			draw_sphere(person.position, 0.01, glm::dvec4{0.0, 1.0, 0.0, 1.0});
			break;

		case Person::infected:
			draw_sphere(person.position, 0.01, glm::dvec4{1.0, 0.0, 0.0, 1.0});
			break;
		case Person::recovered:
			draw_sphere(person.position, 0.01, glm::dvec4{0.7, 0.7, 0.7, 1.0});
			break;
		}
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window);
}

void Renderer::Cleanup()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	window.Destroy();
}

void Renderer::draw_sphere(glm::dvec2 point, double radius, glm::dvec4 color)
{
	basic_shader.SetUniform(
	    "u_model",
	    glm::scale(
	        glm::translate(glm::dmat4{1}, glm::dvec3{point, 0}),
	        glm::dvec3{radius, radius, 1}));
	basic_shader.SetUniform("u_color", color);
	glBindVertexArray(circle_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 2000);
}

glm::dvec2 Renderer::ViewportToScreen(glm::dvec2 in, const SimManager& manager)
{
	return WorldToScreen(ViewportToWorld(in, manager));
}

glm::dvec2 Renderer::ScreenToViewport(glm::dvec2 in, const SimManager& manager)
{
	return WorldToViewport(ScreenToWorld(in), manager);
}

glm::dvec2 Renderer::WorldToScreen(glm::dvec2 in)
{
	return in * window_size;
}

glm::dvec2 Renderer::ScreenToWorld(glm::dvec2 in)
{
	return in / window_size;
}

glm::dvec2 Renderer::WorldToViewport(glm::dvec2 in, const SimManager& manager)
{
	return (in - manager.viewport.xy()) / manager.viewport.zw();
}

glm::dvec2 Renderer::ViewportToWorld(glm::dvec2 in, const SimManager& manager)
{
	glm::dvec2 mouse_pos = in * manager.viewport.zw() + manager.viewport.xy();
}
