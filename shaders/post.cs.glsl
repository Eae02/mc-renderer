#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"
#include "inc/depth.glh"

layout (local_size_x = 32, local_size_y = 32) in;

layout(binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

const float rayleighBrightness = 3.0;
const float rayleighStrength = 700;
const float rayleighCollectionPower = 1.0002;
const float rayleighCollectionScale = 0.003;

const float mieDistribution = 15;
const float mieBrightness = 1.5;
const float mieStrength = 500;
const float mieCollectionScale = 0.02;
const float mieCollectionPower = 1.0002;

const float spotBrightness = 200;
const float scatterStrength = 700;

const float atmosphereRadius = 10000;
const float surfaceLevelPercentage = 0.97;
const float occlusionSmoothness = 0.1;
const float blockDepthScale = 0.5;
const float exposure = 1.0f;

const vec3 absorbtionProfile = vec3(0.18867780436772762, 0.4978442963618773, 0.6616065586417131);

const int sampleCount = 16;

layout(binding=1, rgba8) uniform image2D colorImage;
layout(binding=2) uniform sampler2D depthImage;

vec3 getEyeVector()
{
	vec2 position = vec2(gl_GlobalInvocationID.xy * 2) / vec2(imageSize(colorImage)) - vec2(1.0);
	
	vec4 nearFrustumVertexWS = renderSettings.invViewProj * vec4(position, 0, 1);
	vec4 farFrustumVertexWS = renderSettings.invViewProj * vec4(position, 1, 1);
	
	nearFrustumVertexWS.xyz /= nearFrustumVertexWS.w;
	farFrustumVertexWS.xyz /= farFrustumVertexWS.w;
	
	return normalize(farFrustumVertexWS.xyz - nearFrustumVertexWS.xyz);
}

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

void main()
{
	vec4 color = imageLoad(colorImage, ivec2(gl_GlobalInvocationID.xy));
	float depth = texture(depthImage, gl_GlobalInvocationID.xy).r;
	float linDepth = linearizeDepth(depth);
	
	vec3 eyeVector = getEyeVector();
	
	vec3 cameraPosition = vec3(0.0, atmosphereRadius * surfaceLevelPercentage, 0.0);
	
	const float timescale = 0.5;
	vec3 lightDir = vec3(cos(renderSettings.time * timescale), sin(renderSettings.time * timescale), 0);
	
	float alpha = dot(eyeVector, lightDir);
	
	float rayleighFactor = phase(alpha, -0.01) * rayleighBrightness;
	float mieFactor = phase(alpha, mieDistribution) * mieBrightness;
	float spotFactor = depth > 0.999 ? smoothstep(0.0, 15.0, phase(alpha, 0.9995)) * spotBrightness : 0;
	
	float eyeDepth = min(linDepth * blockDepthScale * (atmosphereRadius / ZFar), getAtmosphericTravelDistance(cameraPosition, eyeVector));
	float eyeRayOcc = depth > 0.999 ? getRayOcclusionAmount(cameraPosition, eyeVector, 0.85) : 1.0;
	
	float sampleStep = eyeDepth / float(sampleCount);
	
	vec3 rayleighLight = vec3(0.0);
	vec3 mieLight = vec3(0.0);
	float occSum = 0.0;
	
	for(int i = 0; i < sampleCount; i++)
	{
		float sampleDistance = sampleStep * float(i);
		vec3 position = cameraPosition + eyeVector * sampleDistance;
		float occlusion = mix(0.1, 1.0, getRayOcclusionAmount(position, lightDir, 0.8));
		float sampleDepth = getAtmosphericTravelDistance(position, lightDir);
		
		vec3 influx = calcAbsorbtion(sampleDepth, vec3(1.0), scatterStrength) * occlusion;
		
		rayleighLight += calcAbsorbtion(sampleDistance, absorbtionProfile * influx, rayleighStrength);
		mieLight += calcAbsorbtion(sampleDistance, influx, mieStrength);
		occSum += occlusion;
	}
	
	rayleighLight = (rayleighLight * eyeRayOcc * (pow(eyeDepth * rayleighCollectionScale, rayleighCollectionPower))) / float(sampleCount);
	mieLight = (mieLight * eyeRayOcc * (pow(eyeDepth * mieCollectionScale, mieCollectionPower))) / float(sampleCount);
	
	color.rgb += mieLight * (spotFactor + mieFactor) + rayleighFactor * rayleighLight;
	color.rgb = vec3(1.0) - exp(-exposure * color.rgb);
	
	imageStore(colorImage, ivec2(gl_GlobalInvocationID.xy), color);
}