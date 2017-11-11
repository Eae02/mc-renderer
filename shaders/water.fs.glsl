#version 440 core
#extension GL_GOOGLE_include_directive : enable

const float indexOfRefraction = 0.6;
const float reflectDistortionFactor = 0.05;
const float visibility = 5;
const vec3 colorExtinction = vec3(0.7, 3.0, 4.0);
const vec3 waterUp = vec3(0, 1, 0);
const float nmStrength = 0.15;

const int nmSamples = 3;

#include "inc/rendersettings.glh"
#include "inc/light.glh"

const float R0 = 0.03;

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(location=0) noperspective in vec2 screenCoord_in;
layout(location=1) in vec3 position_in;
layout(location=2) in vec2 normalMapSamples_in[nmSamples];

layout(location=0) out vec4 color_out;

layout(set=0, binding=1) uniform sampler2D inputColor;
layout(set=0, binding=2) uniform sampler2D inputDepth;

layout(set=0, binding=4) uniform sampler2D normalMap;

vec3 reconstructWorldPos(float depthH, vec2 texCoord)
{
	vec4 h = vec4(texCoord * 2.0 - vec2(1.0), depthH, 1.0);
	vec4 d = renderSettings.invViewProj * h;
	return d.xyz / d.w;
}

vec3 vectorProject(vec3 v, vec3 target)
{
	return target * (dot(target, v) / dot(target, target));
}

vec3 getExtinctionMul(float waterTravelDist)
{
	return max(1.0 - max(waterTravelDist, 1.0) / (colorExtinction * visibility), vec3(0.0));
}

vec3 toWorldSpaceNormal(vec3 normalMapValue)
{
	vec3 nmNormal = (normalMapValue * (255.0 / 128.0)) - vec3(1.0);
	nmNormal.xy *= nmStrength;
	return normalize(nmNormal.xzy);
}

const bool underwater = false;

void main()
{
	vec3 normal = vec3(0, 0, 0);
	for (int i = 0; i < nmSamples; i++)
	{
		normal += toWorldSpaceNormal(texture(normalMap, normalMapSamples_in[i]).xyz);
	}
	normal = normalize(normal);
	
	vec3 targetPos = reconstructWorldPos(texture(inputDepth, screenCoord_in).r, screenCoord_in);
	
	float tDepth = distance(targetPos, renderSettings.cameraPos);
	float wDepth = distance(position_in, renderSettings.cameraPos);
	
	vec3 dirToEye = normalize(renderSettings.cameraPos - position_in);
	
	float waterTravelDist;
	
	if (underwater)
		waterTravelDist = wDepth;
	else
		waterTravelDist = tDepth - wDepth;
	
	vec3 distortMoveVec = waterUp - vectorProject(normal, waterUp);
	
	//The vector to move by to get to the reflection coordinate
	vec3 reflectMoveVec = distortMoveVec * reflectDistortionFactor;
	
	//Finds the reflection coordinate in screen space
	vec4 screenSpaceReflectCoord = renderSettings.viewProj * vec4(position_in + reflectMoveVec, 1.0);
	screenSpaceReflectCoord.xyz /= screenSpaceReflectCoord.w;
	
	//vec3 reflectColor = texture(reflectionMap, (screenSpaceReflectCoord.xy + vec2(1.0)) / 2.0).xyz;
	vec3 reflectColor = pow(vec3(0.309804, 0.713725, 0.95), vec3(2.2)) * renderSettings.sun.radiance;
	
	vec3 surfaceToTarget = normalize(targetPos - position_in);
	
	vec3 refraction = refract(-dirToEye, underwater ? -normal : normal, indexOfRefraction);
	vec3 refractMoveVec = refraction * min(waterTravelDist, 5.0);
	
	vec3 refractPos = position_in + refractMoveVec;
	
	vec4 screenSpaceRefractCoord = renderSettings.viewProj * vec4(refractPos, 1.0);
	screenSpaceRefractCoord.xyz /= screenSpaceRefractCoord.w;
	
	vec2 refractTexcoord = (screenSpaceRefractCoord.xy + vec2(1.0)) / 2.0;
	
	//Reads the target depth at the refracted coordinate
	float refractedTDepth_H = texture(inputDepth, refractTexcoord).r;
	
	vec3 refractColor;
	
	if (refractedTDepth_H < gl_FragCoord.z)
	{
		refractColor = texture(inputColor, screenCoord_in).rgb;
	}
	else
	{
		refractColor = texture(inputColor, refractTexcoord).rgb;
		
		targetPos = reconstructWorldPos(refractedTDepth_H, refractTexcoord);
		
		if (!underwater)
		{
			waterTravelDist = distance(position_in, targetPos);
		}
	}
	
	//refractColor += getCausticsColor(targetPos, RenderSettings.Uptime);
	
	refractColor *= getExtinctionMul(waterTravelDist);
	
	vec3 color = refractColor;
	if (!underwater)
	{
		const float roughness = 0.15;
		
		vec3 fresnel = fresnelSchlick(max(dot(normal, dirToEye), 0.0), vec3(R0), roughness);
		
		color = mix(refractColor, reflectColor, fresnel);
		
		vec3 radiance = renderSettings.sun.radiance * 5;
		
		float NdotL = dot(normal, -renderSettings.sun.direction);
		float NDF = distributionGGX(normal, normalize(dirToEye - renderSettings.sun.direction), roughness);
		
		color += NDF * fresnel * radiance * NdotL;
	}
	
	color_out = vec4(color, 1.0);
}
