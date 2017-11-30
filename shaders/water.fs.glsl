#version 440 core
#extension GL_GOOGLE_include_directive : enable

#define DLS_SET_INDEX 1

#include "inc/rendersettings.glh"
#include "inc/light.glh"

const float R0 = 0.03;
const float nmStrength = 0.15;
const int nmSamples = 3;

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/dlshadow.glh"

layout(location=0) noperspective in vec2 screenCoord_in;
layout(location=1) in vec3 position_in;
layout(location=2) in vec3 scatteringColor_in;
layout(location=3) in mat3 tbnMatrix_in;
layout(location=6) in vec2 normalMapSamples_in[nmSamples];

layout(location=0) out vec4 color_out;

layout(set=0, binding=1) uniform sampler2D inputColor;
layout(set=0, binding=2) uniform sampler2D inputDepth;

layout(set=0, binding=4) uniform sampler2D normalMap;

layout(set=0, binding=5) uniform sampler3D causticsMap;

#include "inc/water.glh"

vec3 reconstructWorldPos(float depthH, vec2 texCoord)
{
	vec4 h = vec4(texCoord * 2.0 - vec2(1.0), depthH, 1.0);
	vec4 d = renderSettings.invViewProj * h;
	return d.xyz / d.w;
}

vec3 toWorldSpaceNormal(vec3 normalMapValue)
{
	vec3 nmNormal = (normalMapValue * (255.0 / 128.0)) - vec3(1.0);
	nmNormal.xy *= nmStrength;
	return normalize(nmNormal.xzy);
}

layout(constant_id=0) const bool underwater = false;

void main()
{
	float hDepth = texture(inputDepth, screenCoord_in).r;
	if (hDepth < gl_FragCoord.z)
		discard;
	
	vec3 targetPos = reconstructWorldPos(hDepth, screenCoord_in);
	
	vec3 normalTS = vec3(0, 0, 0);
	for (int i = 0; i < nmSamples; i++)
	{
		normalTS += (texture(normalMap, normalMapSamples_in[i]).xyz * (255.0 / 128.0)) - vec3(1.0);
	}
	normalTS.xy *= nmStrength;
	vec3 normal = normalize(tbnMatrix_in * normalTS);
	
	float tDepth = distance(targetPos, renderSettings.cameraPos);
	float wDepth = distance(position_in, renderSettings.cameraPos);
	
	vec3 dirToEye = normalize(renderSettings.cameraPos - position_in);
	
	float waterTravelDist = underwater ? wDepth : (tDepth - wDepth);
	
	float horizontalDepth = position_in.y - targetPos.y;
	
	vec3 distortMoveVec = waterUp * (1.0 - dot(normal, waterUp));
	
	//The vector to move by to get to the reflection coordinate
	vec3 reflectMoveVec = distortMoveVec * reflectDistortionFactor;
	
	//Finds the reflection coordinate in screen space
	vec4 screenSpaceReflectCoord = renderSettings.viewProj * vec4(position_in + reflectMoveVec, 1.0);
	screenSpaceReflectCoord.xyz /= screenSpaceReflectCoord.w;
	
	//vec3 reflectColor = texture(reflectionMap, (screenSpaceReflectCoord.xy + vec2(1.0)) / 2.0).xyz;
	vec3 reflectColor = pow(vec3(0.309804, 0.713725, 0.95), vec3(2.2)) * renderSettings.sun.radiance;
	
	vec3 surfaceToTarget = normalize(targetPos - position_in);
	
	vec3 refraction = refract(-dirToEye, underwater ? -normal : normal, indexOfRefraction);
	vec3 refractMoveVec = refraction * min(tDepth - wDepth, 5.0);
	
	vec3 refractPos = position_in + refractMoveVec;
	
	vec4 screenSpaceRefractCoord = renderSettings.viewProj * vec4(refractPos, 1.0);
	screenSpaceRefractCoord.xyz /= screenSpaceRefractCoord.w;
	
	vec2 refractTexcoord = (screenSpaceRefractCoord.xy + vec2(1.0)) / 2.0;
	refractTexcoord = clamp(refractTexcoord, vec2(0.0), vec2(1.0));
	
	//Reads the target depth at the refracted coordinate
	float refractedTDepth_H = texture(inputDepth, refractTexcoord).r;
	
	vec3 refractColor;
	
	if (refractedTDepth_H < gl_FragCoord.z)
	{
		//The refracted coordinate is above water, fall back to the original color instead.
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
	
	if (!underwater && waterTravelDist < 20)
	{
		float targetShadowFactor = getShadowFactor(targetPos);
		if (targetShadowFactor > 0.01)
		{
			refractColor += getCaustics(targetPos, horizontalDepth) * targetShadowFactor;
		}
	}
	
	refractColor = doColorExtinction(refractColor, waterTravelDist, horizontalDepth);
	
	if (!underwater)
	{
		const float roughness = 0.2;
		
		vec3 fresnel = fresnelSchlick(max(dot(normal, dirToEye), 0.0), vec3(R0), roughness);
		
		float surfaceShadowFactor = getShadowFactor(position_in);
		
		MaterialData materialData;
		materialData.albedo = mix(refractColor, reflectColor, fresnel);
		materialData.roughness = roughness;
		materialData.metallic = 0;
		materialData.normal = normal;
		materialData.specularIntensity = 5.0;
		vec3 R = calcDirLightReflectance(renderSettings.sun, dirToEye, fresnel, materialData) * surfaceShadowFactor;
		
		R += getAmbientReflectance(materialData) * (renderSettings.sun.radiance + renderSettings.moon.radiance);
		
		color_out = vec4(R + scatteringColor_in, 1.0);
	}
	else
	{
		color_out = vec4(refractColor, 1.0);
	}
}
