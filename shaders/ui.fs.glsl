#version 440 core

layout(location=0) in vec3 textureCoordinate_in;
layout(location=1) in vec4 color_in;

layout(location=0) out vec4 color_out;

layout(set=0, binding=0) uniform sampler texSampler;

layout(set=1, binding=0) uniform texture2DArray tex;

void main()
{
	color_out = color_in * texture(sampler2DArray(tex, texSampler), textureCoordinate_in);
}