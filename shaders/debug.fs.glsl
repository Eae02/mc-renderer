#version 440 core

layout(location=0) out vec4 color_out;

layout(push_constant) uniform PushConstFS
{
	vec4 color;
} pc;

void main()
{
	color_out = pc.color;
}
