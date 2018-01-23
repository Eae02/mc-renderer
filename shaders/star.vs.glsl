#version 450 core
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec3 color_in;
layout(location=1) in vec4 starPos_in;

layout(location=0) out vec4 directionAndDist_out;
layout(location=1) out vec3 eyeVector_out;
layout(location=2) out vec3 color_out;
layout(location=3) out vec2 screenCoord_out;

const vec2 positions[] = 
{
	vec2(-1, -1),
	vec2(-1, 1),
	vec2(1, -1),
	vec2(-1, 1),
	vec2(1, -1),
	vec2(1, 1)
};

#include "inc/rendersettings.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(push_constant) uniform PC
{
	vec2 halfStarSize;
	float intensityScale;
	mat3 rotation;
};

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_CullDistance[];
};

const vec3 playerPos = vec3(0.0f, 0.5f, 0.0f);

void main()
{
	vec3 toStar = normalize((rotation * starPos_in.xyz) - playerPos);
	
	vec3 viewPos = normalize((renderSettings.view * vec4(toStar, 0.0)).xyz);
	vec4 ndcPos = renderSettings.proj * vec4(viewPos, 0.0);
	vec2 starPos = ndcPos.xy / ndcPos.w;
	
	color_out = color_in * intensityScale;
	directionAndDist_out = vec4(toStar, starPos_in.w);
	
	gl_CullDistance[0] = ndcPos.z;
	gl_Position = vec4(starPos + positions[gl_VertexIndex] * halfStarSize, 0.0, 1.0);
	
	screenCoord_out = (gl_Position.xy + vec2(1.0)) / 2.0;
	
	vec4 nearFrustumVertexWS = renderSettings.invViewProj * gl_Position;
	vec4 farFrustumVertexWS = renderSettings.invViewProj * vec4(gl_Position.xy, 1, 1);
	
	nearFrustumVertexWS.xyz /= nearFrustumVertexWS.w;
	farFrustumVertexWS.xyz /= farFrustumVertexWS.w;
	
	eyeVector_out = normalize((farFrustumVertexWS.xyz - nearFrustumVertexWS.xyz) - playerPos);
}
