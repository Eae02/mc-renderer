#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(set=0, binding=1) uniform sampler2DArray blocksTexture;

layout(location=0) in vec3 textureCoord_in;

layout(location=0) out vec4 color_out;

void main()
{
	color_out = texture(blocksTexture, textureCoord_in);
}
