#version 440 core

layout(location=0) in vec3 textureCoord_in;

layout(set=0, binding=2) uniform sampler2DArray blocksTexture;

void main()
{
	if (texture(blocksTexture, textureCoord_in).a < 0.5)
		discard;
}
