#version 440 core

layout(location=0) in vec3 position_in;

layout(location=0) out vec4 color_out;

void main()
{
	color_out = vec4(0.2, 0.2, 1.0, 0.8);
}
