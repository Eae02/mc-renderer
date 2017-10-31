#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"
#include "inc/depth.glh"
#include "godrays-blur.glh"

layout(location=0) in vec2 texCoord_in;
layout(location=1) in vec3 eyeVector_in;

layout(location=0) out vec3 color_out;

layout(binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(binding=1) uniform sampler2D colorImage;
layout(binding=2) uniform sampler2D depthImage;
layout(binding=3) uniform sampler2D godRaysImage;

const float rayleighBrightness = 0.3;
const float rayleighStrength = 5000;
const float rayleighCollectionPower = 1.0005;
const float rayleighCollectionScale = 0.002;

const float mieDistribution = 10;
const float mieBrightness = 0.3;
const float mieStrength = 1000;
const float mieCollectionScale = 0.05;
const float mieCollectionPower = 1.1;

const float spotBrightness = 50;
const float scatterStrength = 1000;

const float atmosphereRadius = 100000;
const float surfaceLevelPercentage = 0.997;
const float occlusionSmoothness = 0.01;
const float blockDepthScale = 0.05;
const float exposure = 1.0f;

const vec3 absorbtionProfile = vec3(0.18867780436772762, 0.4978442963618773, 0.6616065586417131);

const int sampleCount = 16;

float getAtmosphericTravelDistance(vec3 position, vec3 dir)
{
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, position);
	float c = dot(position, position) - atmosphereRadius * atmosphereRadius;
	float det = b * b - 4.0 * a * c;
	float quadAnswer = (-b - sqrt(det)) / 2.0;
	return (c / quadAnswer);
}

float getRayOcclusionAmount(vec3 position, vec3 dir, float roughness)
{
	float u = dot(dir, -position);
	if (u < 0.0)
		return 1.0;
	float planetRadius = surfaceLevelPercentage * atmosphereRadius * roughness;
	
	vec3 near = position + u * dir;
	if(length(near) < planetRadius)
	{
		return 0.0;
	}
	else
	{
		vec3 v2 = normalize(near) * planetRadius - position;
		float diff = acos(dot(normalize(v2), dir));
		return smoothstep(0.0, 1.0, pow(diff * 2.0, 3.0));
	}
}

float phase(float alpha, float g)
{
	float a = 3.0 * (1.0 - g * g);
	float b = 2.0 * (2.0 + g * g);
	float c = 1.0 + alpha * alpha;
	float d = pow(1.0 + g * g - 2.0 * g * alpha, 1.5);
	return (a / b) * (c / d);
}

vec3 calcAbsorbtion(float dist, vec3 initialColor, float factor)
{
	return initialColor - initialColor * pow(absorbtionProfile, vec3(factor / dist));
}

layout(push_constant) uniform PC
{
	float yPixelSize;
} pc;

void main()
{
	vec4 color = texture(colorImage, texCoord_in);
	float depth = texture(depthImage, texCoord_in).r;
	float linDepth = linearizeDepth(depth);
	
	vec3 eyeVector = normalize(eyeVector_in);
	
	vec3 cameraPosition = vec3(0.0, atmosphereRadius * surfaceLevelPercentage, 0.0);
	
	float alpha = dot(eyeVector, -renderSettings.sun.direction);
	
	const bool sky = depth >= (1.0 - 1E-9);
	
	float rayleighFactor = phase(alpha, -0.01) * rayleighBrightness;
	float mieFactor = phase(alpha, mieDistribution) * mieBrightness;
	//float spotFactor = sky ? smoothstep(0.0, 15.0, phase(alpha, 0.9995)) * spotBrightness : 0;
	
	float eyeDepth = min(linDepth * blockDepthScale * (atmosphereRadius / ZFar), getAtmosphericTravelDistance(cameraPosition, eyeVector));
	float eyeRayOcc = sky ? getRayOcclusionAmount(cameraPosition, eyeVector, 0.85) : 1.0;
	
	float sampleStep = eyeDepth / float(sampleCount);
	
	vec3 rayleighLight = vec3(0.0);
	vec3 mieLight = vec3(0.0);
	float occSum = 0.0;
	
	for(int i = 0; i < sampleCount; i++)
	{
		float sampleDistance = sampleStep * float(i);
		vec3 position = cameraPosition + eyeVector * sampleDistance;
		float occlusion = mix(0.1, 1.0, getRayOcclusionAmount(position, -renderSettings.sun.direction, 0.8));
		float sunSampleDepth = getAtmosphericTravelDistance(position, -renderSettings.sun.direction);
		float moonSampleDepth = getAtmosphericTravelDistance(position, -renderSettings.moon.direction);
		
		vec3 influx = calcAbsorbtion(sunSampleDepth, renderSettings.sun.radiance, scatterStrength) * occlusion;
		
		influx += calcAbsorbtion(moonSampleDepth, renderSettings.moon.radiance, scatterStrength);
		
		rayleighLight += calcAbsorbtion(sampleDistance, absorbtionProfile * influx, rayleighStrength);
		mieLight += calcAbsorbtion(sampleDistance, influx, mieStrength);
		occSum += occlusion;
	}
	
	rayleighLight = (rayleighLight * eyeRayOcc * (pow(eyeDepth * rayleighCollectionScale, rayleighCollectionPower))) / float(sampleCount);
	mieLight = (mieLight * eyeRayOcc * (pow(eyeDepth * mieCollectionScale, mieCollectionPower))) / float(sampleCount);
	
	color_out = color.rgb + mieLight * mieFactor + rayleighFactor * rayleighLight;
	
	color_out += sampleAndBlurGodRays(godRaysImage, texCoord_in, vec2(0.0, pc.yPixelSize)) * renderSettings.sun.radiance;
	
	color_out = vec3(1.0) - exp(-exposure * color_out);
}