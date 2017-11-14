#version 440 core
#extension GL_GOOGLE_include_directive : enable

#define DLS_SET_INDEX 1

#include "inc/rendersettings.glh"
#include "inc/light.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/dlshadow.glh"

layout(location=0) in vec2 screenCoord_in;

layout(location=0) out vec4 color_out;

layout(set=0, binding=1) uniform sampler2D inputColor;
layout(set=0, binding=2) uniform sampler2D inputDepth;

layout(set=0, binding=3) uniform sampler3D causticsMap;

#include "inc/water.glh"

layout(push_constant) uniform PC
{
	bool underwater;
	float waterY;
};

vec3 reconstructWorldPos(float depthH, vec2 texCoord)
{
	vec4 h = vec4(texCoord * 2.0 - vec2(1.0), depthH, 1.0);
	vec4 d = renderSettings.invViewProj * h;
	return d.xyz / d.w;
}

void main()
{
	vec3 color = texture(inputColor, screenCoord_in).rgb;
	gl_FragDepth = texture(inputDepth, screenCoord_in).r;
	
	if (underwater)
	{
		vec3 targetPos = reconstructWorldPos(gl_FragDepth, screenCoord_in);
		
		float horizontalDepth = waterY - targetPos.y;
		float waterTravelDist = distance(targetPos, renderSettings.cameraPos);
		
		if (waterTravelDist < 20)
		{
			float shadowFactor = getShadowFactor(targetPos);
			if (shadowFactor > 0.01)
			{
				color += getCaustics(targetPos, horizontalDepth) * shadowFactor;
			}
		}
		
		color = doColorExtinction(color, waterTravelDist, horizontalDepth);
	}
	
	color_out = vec4(color, 1.0);
}
