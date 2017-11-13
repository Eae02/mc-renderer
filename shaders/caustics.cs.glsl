#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/perlin.glh"

layout(constant_id=0) gl_WorkGroupSize;
layout(constant_id=1) const float positionScaleXY = 1;
layout(constant_id=2) const float positionScaleZ = 1;

layout(local_size_x=8, local_size_y=8, local_size_z=8) in;

layout(set=0, binding=0, r8) uniform image3D resultImage;

const int octaves = 3;
const float gain = 1.5; //Increasing makes higher octaves rougher
const float lacunarity = 0.75; //Increasing makes higher octaves influence the result more

void main()
{
	vec3 position = vec3(vec2(gl_GlobalInvocationID.xy) * positionScaleXY,
	                     float(gl_GlobalInvocationID.z) * positionScaleZ);
	
	float ridgeNoise = 1.0 - perlinOctRidge(position, octaves, gain, lacunarity);
	
	imageStore(resultImage, ivec3(gl_GlobalInvocationID), vec4(ridgeNoise));
}
