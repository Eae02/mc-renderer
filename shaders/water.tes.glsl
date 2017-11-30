#version 440 core
#extension GL_GOOGLE_include_directive : enable

layout(triangles, fractional_odd_spacing, cw) in;

#include "inc/rendersettings.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/water-vertex.glh"

layout(location=0) in vec3 position_in[];
layout(location=1) in float depth_in[];
layout(location=2) in vec3 scatteringColor_in[];

layout(location=0) noperspective out vec2 screenCoord_out;
layout(location=1) out vec3 position_out;
layout(location=2) out vec3 scatteringColor_out;
layout(location=3) out mat3 tbnMatrix_out;
layout(location=6) out vec2 normalMapSamples_out[nmSamples];

vec3 interpolate3(vec3 v0, vec3 v1, vec3 v2)
{
	return (gl_TessCoord.x * v0) + (gl_TessCoord.y * v1) + (gl_TessCoord.z * v2);
}

void main()
{
	position_out = interpolate3(position_in[0], position_in[1], position_in[2]);
	scatteringColor_out = interpolate3(scatteringColor_in[0], scatteringColor_in[1], scatteringColor_in[2]);
	
	float depth = gl_TessCoord.x * depth_in[0] + gl_TessCoord.y * depth_in[1] + gl_TessCoord.z * depth_in[2];
	
	calcWaves(position_out, tbnMatrix_out, depth);
	
	for (int i = 0; i < nmSamples; i++)
	{
		normalMapSamples_out[i] = calcNMSample(i, position_out);
	}
	
	gl_Position = renderSettings.viewProj * vec4(position_out, 1.0);
	screenCoord_out = (gl_Position.xy / gl_Position.w) * 0.5 + 0.5;
}
