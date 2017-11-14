#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"
#include "inc/depth.glh"
#include "godrays-blur.glh"

layout(location=0) in vec2 texCoord_in;
layout(location=1) in vec3 eyeVector_in;

layout(location=0) out vec4 color_out;

layout(binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/atmosphere.glh"

layout(binding=1) uniform sampler2D depthImage;
layout(binding=2) uniform sampler2D godRaysImage;

layout(push_constant) uniform PC
{
	float yPixelSize;
} pc;

void main()
{
	vec3 eyeVector = normalize(eyeVector_in);
	
	float hDepth = texture(depthImage, texCoord_in).r;
	float linDepth = linearizeDepth(hDepth);
	float eyeDepth = min(blockDepthToAtmosphereDepth(linDepth),
	                     getAtmosphericTravelDistance(atmosphereCameraPos, eyeVector));
	
	color_out = vec4(getScatteringColor(eyeDepth, normalize(eyeVector)), 1.0);
	
	if (hDepth == 1.0)
	{
		color_out.rgb *= getRayOcclusionAmount(atmosphereCameraPos, eyeVector, 0.85);
	}
	
	color_out.rgb += sampleAndBlurGodRays(godRaysImage, texCoord_in, vec2(0.0, pc.yPixelSize)) * renderSettings.sun.radiance;
}
