#version 440 core
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 texCoord_in;

layout(binding=0) uniform sampler2D unblurredImage;

layout(location=0) out float light_out;

#include "godrays-blur.glh"

layout(push_constant) uniform PC
{
	float xPixelSize;
} pc;

void main()
{
	light_out = sampleAndBlurGodRays(unblurredImage, texCoord_in, vec2(pc.xPixelSize, 0.0));
}
