#version 430 core

in vec4 frag_color;

out vec4 out_color;

uniform sampler2D u_Texture;

void main()
{
	out_color = frag_color;
}
