#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

const uint sampleCount = 64;
const float brightness = 5;
const float lightDecay = pow(0.00004, 1.0 / float(sampleCount));
const float radius = 15;

layout(binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(binding=1) uniform sampler2D depthSampler;

layout(location=0) in vec2 texCoord_in;

layout(location=0) out float light_out;

void main()
{
	const float viewSize = 700;
	
	vec2 texSize = textureSize(depthSampler, 0);
	vec2 coordMul = vec2(viewSize, viewSize * (texSize.y / texSize.x));;
	
	float distToLight = distance(texCoord_in * coordMul, renderSettings.sunScreenPosition * coordMul);
	float lightBeginSample = max(1.0 - (radius / distToLight), 0.0) * sampleCount;
	
	float illuminationDecay = pow(lightDecay, floor(lightBeginSample) + 1);
	
	float light = 0.0;
	
	for (uint i = uint(ceil(lightBeginSample)); i < sampleCount; i++)
	{
		vec2 sampleCoord = mix(texCoord_in, renderSettings.sunScreenPosition, float(i) / float(sampleCount));
		
		float depth = texture(depthSampler, sampleCoord).r;
		
		if (depth > 1.0 - 1E-6)
		{
			light += illuminationDecay;
		}
		illuminationDecay *= lightDecay;
	}
	
	light_out = light * brightness;
}
