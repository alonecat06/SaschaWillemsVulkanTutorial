#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout (set = 0, binding = 0) uniform UBOScene
{
	mat4 projection;
	mat4 view;
	vec4 lightPos;
	vec4 viewPos;
} uboScene;

layout(push_constant) uniform PushConsts
{
	float len;
	float ratio;
} fur;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

void main() 
{
	outColor = inColor;
	outUV = inUV;
	outNormal = mat3(uboScene.view) * inNormal;
	
	vec4 pos = uboScene.view * vec4(inPos.xyz + fur.ratio * fur.len * outNormal, 1.0);
	outLightVec = uboScene.lightPos.xyz - pos.xyz;
	outViewVec = uboScene.viewPos.xyz - pos.xyz;
	
	gl_Position = uboScene.projection * pos;
}