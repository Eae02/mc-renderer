#version 440 core

const uint NumCascades = 3;

layout(triangles) in;
layout(invocations=NumCascades) in;

layout(triangle_strip) out;
layout(max_vertices=3) out;

layout(location=0) out vec3 textureCoord_out;

layout(location=0) in vec3 textureCoord_in[];
layout(location=1) in vec3 worldPos_in[];

layout(set=0, binding=1) uniform LightMatricesUB
{
	mat4 lightMatrices[NumCascades];
};

void main()
{
	for (int i = 0; i < 3; i++)
	{
		gl_Layer = gl_InvocationID;
		
		textureCoord_out = textureCoord_in[i];
		
		gl_Position = lightMatrices[gl_InvocationID] * vec4(worldPos_in[i], 1.0);
		gl_Position.z = clamp(gl_Position.z, 0.0, 1.0);
		
		EmitVertex();
	}
	
	EndPrimitive();
}
