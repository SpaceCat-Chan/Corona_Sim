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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_CULL_FACE);

	return true;
}

void Renderer::StartImGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
}

void Renderer::Draw(const SimManager &manager, glm::dvec2 mouse)
{
	glm::dmat4 view_proj
	    = glm::ortho(0.0, 1.0, 1.0, 0.0)
	      * glm::translate(glm::dmat4{1}, glm::dvec3{manager.viewport.xy(), 0})
	      * glm::scale(glm::dmat4{1}, glm::dvec3{manager.viewport.zw(), 1});
	basic_shader.SetUniform("u_view_proj", view_proj);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	auto &map = manager.m_world.get_layout();
	if (manager.viewing_floor_or_group.index() == 0)
	{
		auto floor = std::get<0>(manager.viewing_floor_or_group);
		if (map.contains(floor))
		{
			for (size_t i = 0; i < map.at(floor).obstacles.size(); i++)
			{
				auto &obstacle = map.at(floor).obstacles[i];
				draw_rectangle(
				    obstacle.position,
				    obstacle.size,
				    obstacle.rotation,
				    {0.5, 0.5, 0.5, 1.0});
			}
		}
	}
	else
	{
		for (auto &floor : map)
		{
			if (floor.second.group == ""
			    || floor.second.group
			           != std::get<1>(manager.viewing_floor_or_group))
			{
				continue;
			}
			for (auto obstacle : floor.second.obstacles)
			{
				draw_rectangle(
				    obstacle.position,
				    obstacle.size,
				    obstacle.rotation,
				    {0.5, 0.5, 0.5, 1.0});
			}
		}
	}

	for (auto &person : manager.SimRunning ? manager.m_current_people
	                                       : manager.m_simulation_start_people)
	{
		if (manager.viewing_floor_or_group.index() == 0)
		{
			if (person.floor != std::get<0>(manager.viewing_floor_or_group))
			{
				continue;
			}
		}
		else
		{
			if (map.contains(person.floor))
			{
				if (map.at(person.floor).group
				        != std::get<1>(manager.viewing_floor_or_group)
				    || map.at(person.floor).group == "")
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
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

	for (auto &changer : manager.m_world.get_floor_changers())
	{
		glLineWidth(10);
		if (manager.is_visable(changer.a.first))
		{
			draw_sphere(changer.a.second, 0.005, {0.2, 0, 0.7, 1});
			draw_sphere(changer.a.second, 0.005, {0.1, 0, 0.6, 1}, true);
		}
		if (manager.is_visable(changer.b.first))
		{
			draw_sphere(changer.b.second, 0.005, {0.2, 0, 0.7, 1});
			draw_sphere(changer.b.second, 0.005, {0.1, 0, 0.6, 1}, true);
		}
		glLineWidth(1);
	}

	if (manager.m_selection_box)
	{
		for (auto &selected : *manager.m_selection_box)
		{
			if (selected.index() == 0)
			{
				auto &people = manager.SimRunning
				                   ? manager.m_current_people
				                   : manager.m_simulation_start_people;
				auto &person = people[std::get<0>(selected)];
				if (manager.is_visable(person.floor))
				{
					draw_rectangle(
					    person.position - glm::dvec2{0.01},
					    glm::dvec2{0.02},
					    0,
					    {1, 0, 0, 1},
					    true);
				}
			}
			else if (selected.index() == 1)
			{
				auto obstacle_index = std::get<1>(selected);
				if (manager.is_visable(obstacle_index.first))
				{
					auto &obstacle = manager.m_world.get_obstacle(
					    obstacle_index.first,
					    obstacle_index.second);
					auto corner = obstacle.position - glm::dvec2{0.04};
					auto size = obstacle.size + glm::dvec2{0.08};
					draw_rectangle(
					    corner,
					    size,
					    obstacle.rotation,
					    {1, 0, 0, 1},
					    true);
					if (manager.m_selection_box->size() == 1)
					{
						auto main_vertecies = obstacle.get_vertecies(0.04);
						for (size_t i = 0; i < main_vertecies.size(); i += 2)
						{
							auto next = i + 1;
							if (next == main_vertecies.size())
							{
								next = 0;
							}
							auto insert_point = (main_vertecies.begin() + i) + 1;
							main_vertecies.insert(
							    insert_point,
							    glm::mix(
							        main_vertecies[i],
							        main_vertecies[next],
							        0.5));
						}
						for (auto &vertex : main_vertecies)
						{
							draw_rectangle(
							    vertex - glm::dvec2{0.005},
							    {0.01, 0.01},
							    obstacle.rotation,
							    {0.65, 0.65, 0.65, 1});
						}
					}
				}
			}
			else
			{
				auto changer_index = std::get<2>(selected);
				auto &changer
				    = manager.m_world.get_floor_changers()[changer_index.first];
				if (manager.is_visable(changer.a.first))
				{
					draw_rectangle(
					    changer.a.second - glm::dvec2{0.01},
					    glm::dvec2{0.02},
					    0,
					    changer_index.second ? glm::dvec4{1, 0, 0, 1}
					                         : glm::dvec4{0, 1, 0, 1},
					    true);
				}
				if (manager.is_visable(changer.b.first))
				{
					draw_rectangle(
					    changer.b.second - glm::dvec2{0.01},
					    glm::dvec2{0.02},
					    0,
					    !changer_index.second ? glm::dvec4{1, 0, 0, 1}
					                          : glm::dvec4{0, 1, 0, 1},
					    true);
				}
			}
		}
	}

	if (manager.DragState == SimManager::CreateObstacle)
	{
		//this is complex
		//the above comment is fucking wrong, dumbass

		glm::dvec2 min, max;
		min.x = glm::min(manager.last_drag.x, mouse.x);
		min.y = glm::min(manager.last_drag.y, mouse.y);
		max.x = glm::max(manager.last_drag.x, mouse.x);
		max.y = glm::max(manager.last_drag.y, mouse.y);
		draw_rectangle(min, max - min, 0, {0.5, 0.5, 0.5, 0.5});
	}
	else if (manager.DragState == SimManager::Select)
	{
		glm::dvec2 min, max;
		min.x = glm::min(manager.last_drag.x, mouse.x);
		min.y = glm::min(manager.last_drag.y, mouse.y);
		max.x = glm::max(manager.last_drag.x, mouse.x);
		max.y = glm::max(manager.last_drag.y, mouse.y);
		glLineWidth(3);
		draw_rectangle(min, max - min, 0, {0.25, 0.25, 0.25, 0.25}, true);
		glLineWidth(1);
	}
	else
	{
		switch (manager.CreateNext)
		{
		case SimManager::Create::None:
			break;
		case SimManager::Create::Person:
			draw_sphere(mouse, 0.01, {0.0, 1.0, 0.0, 0.5});
			break;
		case SimManager::Create::Obstacle:
			draw_rectangle(
			    mouse - glm::dvec2{0.02},
			    {0.04, 0.04},
			    0,
			    {0.5, 0.5, 0.5, 0.5});
			break;
		case SimManager::Create::Changer:
			glLineWidth(10);
			draw_sphere(mouse, 0.005, {0.2, 0, 0.7, 0.5});
			draw_sphere(mouse, 0.005, {0.1, 0, 0.6, 0.5}, true);
			glLineWidth(1);
			break;
		case SimManager::Create::Paste:
			for (auto &thing : *manager.m_paste_buffer)
			{
				if (thing.index() == 0)
				{
					auto &person = std::get<0>(thing);
					switch (person.state)
					{
					case Person::susceptible:
						draw_sphere(
						    person.position + mouse,
						    0.01,
						    glm::dvec4{0.0, 1.0, 0.0, 0.5});
						break;

					case Person::infected:
						draw_sphere(
						    person.position + mouse,
						    0.01,
						    glm::dvec4{1.0, 0.0, 0.0, 0.5});
						break;
					case Person::recovered:
						draw_sphere(
						    person.position + mouse,
						    0.01,
						    glm::dvec4{0.7, 0.7, 0.7, 0.5});
						break;
					}
				}
				else
				{
					auto &obstacle = std::get<1>(thing);
					draw_rectangle(
					    obstacle.position + mouse,
					    obstacle.size,
					    obstacle.rotation,
					    {0.5, 0.5, 0.5, 0.5});
				}
			}
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

void Renderer::draw_sphere(
    glm::dvec2 point,
    double radius,
    glm::dvec4 color,
    bool line)
{
	basic_shader.SetUniform(
	    "u_model",
	    glm::scale(
	        glm::translate(glm::dmat4{1}, glm::dvec3{point, 0}),
	        glm::dvec3{radius, radius, 1}));
	basic_shader.SetUniform("u_color", color);
	glBindVertexArray(circle_vao);
	if (line)
	{
		glDrawArrays(GL_LINE_LOOP, 0, 2000);
	}
	else
	{
		glDrawArrays(GL_TRIANGLE_FAN, 0, 2000);
	}
}

void Renderer::draw_rectangle(
    glm::dvec2 corner,
    glm::dvec2 size,
    double rotation,
    glm::dvec4 color,
    bool line)
{
	basic_shader.SetUniform(
	    "u_model",
	    glm::scale(
	        glm::rotate(
	            glm::translate(
	                glm::dmat4{1},
	                glm::dvec3{corner + size / 2.0, 0}),
	            rotation,
	            glm::dvec3{0, 0, 1}),
	        glm::dvec3{size, 1}));
	basic_shader.SetUniform("u_color", color);
	glBindVertexArray(square_vao);
	if (line)
	{
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}
	else
	{
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

glm::dvec2 Renderer::WorldToScreen(glm::dvec2 in, const SimManager &manager)
{
	return ViewportToScreen(WorldToViewport(in, manager));
}

glm::dvec2 Renderer::ScreenToWorld(glm::dvec2 in, const SimManager &manager)
{
	return ViewportToWorld(ScreenToViewport(in), manager);
}

glm::dvec2 Renderer::ViewportToScreen(glm::dvec2 in) { return in * window_size; }

glm::dvec2 Renderer::ScreenToViewport(glm::dvec2 in) { return in / window_size; }

glm::dvec2 Renderer::WorldToViewport(glm::dvec2 in, const SimManager &manager)
{
	return in * manager.viewport.zw() + manager.viewport.xy();
}

glm::dvec2 Renderer::ViewportToWorld(glm::dvec2 in, const SimManager &manager)
{
	return (in - manager.viewport.xy()) / manager.viewport.zw();
}
