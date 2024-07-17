#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 16) out;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 viewPos;
} ubo;

layout (location = 0) in vec3 inNormal[];

layout (location = 0) out vec3 outColor;

void main(void)
{	
	float normalLength = 0.2;

	vec3 posCenter = vec3(0,0,0);
	vec3 normalCenter = vec3(0,0,0);
	for(int i=0; i<gl_in.length(); i++)
	{
		vec3 pos = gl_in[i].gl_Position.xyz;
		vec3 normal = inNormal[i].xyz;

		posCenter += pos;
		normalCenter += normal;

//		gl_Position = ubo.projection * (ubo.model * vec4(pos, 1.0));
//		outColor = vec3(1.0, 0.0, 0.0);
//		EmitVertex();
//
//		gl_Position = ubo.projection * (ubo.model * vec4(pos + normal * normalLength, 1.0));
//		outColor = vec3(0.0, 0.0, 1.0);
//		EmitVertex();
//
//		EndPrimitive();
	}

	posCenter /= 3;
	//	normalCenter = normalize(normalCenter);
	vec3 pos0 = (ubo.model * gl_in[0].gl_Position).xyz;
	vec3 pos1 = (ubo.model * gl_in[1].gl_Position).xyz;
	vec3 pos2 = (ubo.model * gl_in[2].gl_Position).xyz;
	vec3 line01 = pos1 - pos0;
	vec3 line02 = pos2 - pos0;
	normalCenter = normalize(cross(line01, line02));
	
	gl_Position = ubo.projection * (ubo.model * vec4(posCenter, 1.0));
	outColor = vec3(0.0, 1.0, 0.0);
	EmitVertex();

	gl_Position = ubo.projection * (ubo.model * vec4(posCenter, 1.0) + vec4(normalCenter * normalLength, 1));
	outColor = vec3(0.0, 1.0, 1.0);
	EmitVertex();

	EndPrimitive();

	gl_Position = ubo.projection * (ubo.model * vec4(posCenter, 1.0));
	outColor = vec3(0.0, 1.0, 0.0);
	EmitVertex();

	gl_Position = ubo.projection * vec4(0, 0, 0, 1);
	outColor = vec3(1.0, 1.0, 0.0);
	EmitVertex();

	EndPrimitive();
}