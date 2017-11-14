#version 440 core
#extension GL_GOOGLE_include_directive : enable

const vec2 positions[] = vec2[]
(
	vec2(-1, -1),
	vec2(-1,  3),
	vec2( 3, -1)
);

layout(location=0) out vec2 texCoord_out;

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
	texCoord_out = gl_Position.xy * 0.5 + vec2(0.5);
}
