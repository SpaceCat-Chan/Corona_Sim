#version 430 core

layout(location = 0) in vec2 in_position;

uniform mat4 u_view_proj;
uniform mat4 u_model;
uniform vec4 u_color;

out vec4 frag_color;

void main()
{
	vec3 vert_position = vec3(u_model * vec4(in_position, 0, 1));
	gl_Position = u_view_proj * vec4(vert_position, 1);
	frag_color = u_color;
}
