#version 450 core
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec4 directionAndDist_in;
layout(location=1) in vec3 eyeVector_in;
layout(location=2) in vec3 color_in;
layout(location=3) in vec2 screenCoord_in;

layout(location=0) out vec4 color_out;

layout(binding=1) uniform sampler2D depthSampler;

void main()
{
	if (texture(depthSampler, screenCoord_in).r < 1.0)
		discard;
	
	float alpha = dot(normalize(directionAndDist_in.xyz), normalize(eyeVector_in));
	
	float factor = pow(clamp(alpha, 0.0, 1.0), directionAndDist_in.w);
	
	color_out.rgb = factor * color_in;
	color_out.a = 0.0;
}
