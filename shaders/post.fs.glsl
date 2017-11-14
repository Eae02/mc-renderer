#version 440 core
#extension GL_GOOGLE_include_directive : enable

const float exposure = 1.2f;

layout(location=0) in vec2 texCoord_in;

layout(location=0) out vec4 color_out;

layout(set=0, binding=0) uniform sampler2D inputSampler;

void main()
{
	vec3 color = texture(inputSampler, texCoord_in).rgb;
	
	color_out = vec4(vec3(1.0) - exp(-exposure * color), 1.0);
}
