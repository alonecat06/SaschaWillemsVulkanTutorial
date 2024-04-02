#version 450

layout (set = 1, binding = 0) uniform sampler2D samplerBaseColor;
layout (set = 1, binding = 1) uniform sampler2D samplerFurMap;

layout (location = 0) in GS_TO_FS
{
	vec3 normal;
	vec2 baseUv;
	vec2 finUv;
	vec3 viewVec;
	vec3 lightVec;
	float lodBias;
} frag_in;

layout(push_constant) uniform PushConsts {
	float alphaCutout;
	float occlusion;
} furFrag;

layout (location = 0) out vec4 outFragColor;

void main()
{
	vec4 furColor = texture(samplerFurMap, frag_in.finUv, frag_in.lodBias);
	if (frag_in.finUv.z == 0 && furColor.a < furFrag.alphaCutout)
	{
		discard;
	}

	outFragColor = texture(samplerBaseColor, frag_in.baseUv, frag_in.lodBias);
	outFragColor.rgb *= lerp(1.0 - furFrag.occlusion, 1.0, frag_in.finUv.y);
}