#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

const vec2 positions[] = vec2[]
(
	vec2(-1, -1),
	vec2(-1,  3),
	vec2( 3, -1)
);

layout(location=0) out vec2 texCoord_out;
layout(location=1) out vec3 eyeVector_out;

layout(binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
	texCoord_out = gl_Position.xy * 0.5 + vec2(0.5);
	
	vec4 nearFrustumVertexWS = renderSettings.invViewProj * gl_Position;
	vec4 farFrustumVertexWS = renderSettings.invViewProj * vec4(gl_Position.xy, 1, 1);
	
	nearFrustumVertexWS.xyz /= nearFrustumVertexWS.w;
	farFrustumVertexWS.xyz /= farFrustumVertexWS.w;
	
	eyeVector_out = farFrustumVertexWS.xyz - nearFrustumVertexWS.xyz;
}