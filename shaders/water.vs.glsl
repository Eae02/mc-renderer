#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/water-vertex.glh"
#include "inc/atmosphere.glh"

layout(location=0) in vec4 positionAndDepth_in;

layout(location=0) noperspective out vec2 screenCoord_out;
layout(location=1) out vec3 position_out;
layout(location=2) out vec3 scatteringColor_out;
layout(location=3) out mat3 tbnMatrix_out;
layout(location=6) out vec2 normalMapSamples_out[nmSamples];

layout(constant_id=0) const bool underwater = false;

void main()
{
	vec3 worldPos = positionAndDepth_in.xyz;
	
	calcWaves(worldPos, tbnMatrix_out, positionAndDepth_in.w);
	
	position_out = worldPos;
	
	if (!underwater)
	{
		vec3 eyeVector = worldPos - renderSettings.cameraPos;
		float distToCamera = length(eyeVector);
		scatteringColor_out = getScatteringColor(blockDepthToAtmosphereDepth(distToCamera), eyeVector / distToCamera);
	}
	
	for (int i = 0; i < nmSamples; i++)
	{
		normalMapSamples_out[i] = calcNMSample(i, worldPos);
	}
	
	gl_Position = renderSettings.viewProj * vec4(worldPos.xyz, 1.0);
	screenCoord_out = (gl_Position.xy / gl_Position.w) * 0.5 + 0.5;
}
