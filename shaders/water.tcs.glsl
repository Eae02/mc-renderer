#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(vertices=3) out;

const float tessellationAmount = 3.0;
const float lowestLodDist = 80.0;

layout(location=0) in vec3 position_in[];
layout(location=1) in float depth_in[];
layout(location=2) in vec3 scatteringColor_in[];

layout(location=0) out vec3 position_out[];
layout(location=1) out float depth_out[];
layout(location=2) out vec3 scatteringColor_out[];

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

float getEdgeTessLevel(float dist1, float dist2)
{
	float avgDist = (dist1 + dist2) / 2.0;
	return mix(7.0, 1.0, min(avgDist / lowestLodDist, 1.0)) * tessellationAmount;
}

void main()
{
	position_out[gl_InvocationID] = position_in[gl_InvocationID];
	depth_out[gl_InvocationID] = depth_in[gl_InvocationID];
	scatteringColor_out[gl_InvocationID] = scatteringColor_in[gl_InvocationID];
	
	float vertexDistances[3] = float[3]
	(
		distance(renderSettings.cameraPos, position_in[0]),
		distance(renderSettings.cameraPos, position_in[1]),
		distance(renderSettings.cameraPos, position_in[2])
	);
	
	gl_TessLevelOuter[gl_InvocationID] = getEdgeTessLevel(vertexDistances[(gl_InvocationID + 1) % 3], vertexDistances[(gl_InvocationID + 2) % 3]);
	
	if (gl_InvocationID == 0)
	{
		float avgDist = (vertexDistances[0] + vertexDistances[1] + vertexDistances[2]) / 3.0;
		gl_TessLevelInner[0] = mix(4.0, 0.0, min(avgDist / lowestLodDist, 1.0)) * tessellationAmount;
	}
}
