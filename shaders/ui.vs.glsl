#version 440 core

layout(location=0) in vec2 position_in;
layout(location=1) in vec3 textureCoordinate_in;
layout(location=2) in vec4 color_in;

layout(location=0) out vec3 textureCoordinate_out;
layout(location=1) out vec4 color_out;

layout(push_constant) uniform PushConstants
{
	vec2 positionScale;
} pc;

void main()
{
	textureCoordinate_out = textureCoordinate_in;
	//textureCoordinate_out.y = 1.0 - textureCoordinate_in.y;
	
	color_out = color_in;
	
	gl_Position = vec4(position_in * pc.positionScale - vec2(1.0), 0, 1);
}