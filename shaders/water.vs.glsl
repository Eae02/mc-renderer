#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

const int nmSamples = 3;

layout(location=0) in vec3 position_in;

layout(location=0) noperspective out vec2 screenCoord_out;
layout(location=1) out vec3 position_out;
layout(location=2) out vec3 scatteringColor_out;
layout(location=3) out vec2 normalMapSamples_out[nmSamples];

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/atmosphere.glh"

layout(set=0, binding=3) uniform NMRotationMatricesUB
{
	mat2 nmRotationMatrices[nmSamples];
};

layout(push_constant) uniform PC
{
	bool underwater;
};

void main()
{
	position_out = position_in;
	
	if (!underwater)
	{
		vec3 eyeVector = position_in - renderSettings.cameraPos;
		float distToCamera = length(eyeVector);
		scatteringColor_out = getScatteringColor(blockDepthToAtmosphereDepth(distToCamera), eyeVector / distToCamera);
	}
	
	const float scales[nmSamples] = float[] (8, 15, 6);
	const float nmScale = 2;
	
	for (int i = 0; i < nmSamples; i++)
	{
		normalMapSamples_out[i] = (nmRotationMatrices[i] * position_in.xz + vec2(0, renderSettings.time)) / (scales[i] * nmScale);
	}
	
	gl_Position = renderSettings.viewProj * vec4(position_in.xyz, 1.0);
	screenCoord_out = (gl_Position.xy / gl_Position.w) * 0.5 + 0.5;
}
