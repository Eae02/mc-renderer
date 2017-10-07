#version 440 core

layout(location=0) in vec3 position_in;
layout(location=1) in vec3 textureCoord_in;

layout(location=0) out vec3 textureCoord_out;
layout(location=1) out vec3 worldPos_out;

void main()
{
	textureCoord_out = textureCoord_in;
	worldPos_out = position_in;
}
