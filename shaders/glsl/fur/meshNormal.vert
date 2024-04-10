#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out VS_TO_GS
{
	vec3 normal;
	vec2 uv;
} vertex_out;

void main() 
{
	gl_Position = vec4(inPos, 1.0);

	vertex_out.normal = inNormal;
	vertex_out.uv = inUV;
}