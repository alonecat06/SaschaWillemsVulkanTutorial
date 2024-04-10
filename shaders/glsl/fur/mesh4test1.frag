#version 450
layout (set = 0, binding = 2) uniform sampler2D samplerBaseColor;
layout (set = 0, binding = 3) uniform sampler2D samplerFinMap;
layout (set = 0, binding = 4) uniform sampler2D samplerNoiseMap;

layout (location = 0) in GS_TO_FS
{
	vec3 normal;
	vec2 baseUv;
	vec2 furUv;
	float shellHigh;
	vec3 viewVec;
	vec3 lightVec;
	float lodBias;
} frag_in;

layout(push_constant) uniform PushConsts {
	float density;
	float attenuation;
	float thickness;
	float occlusion;
	float finCutout;
	float shellCutout;
} furFrag;

layout (location = 0) out vec4 outFragColor;

float hash(vec2 uv){
	return fract(sin(7.289 * uv.x + 11.23 * uv.y) * 23758.5453);
}

void main()
{
	vec4 noiseColor = texture(samplerNoiseMap, frag_in.furUv, frag_in.lodBias);
	float alpha = noiseColor.r * (1.0 - frag_in.shellHigh);
	if (frag_in.shellHigh > 0 && alpha < furFrag.shellCutout)
	{
		discard;
	}

	outFragColor = texture(samplerBaseColor, frag_in.baseUv, frag_in.lodBias);
	outFragColor.rgb *= mix(1 - furFrag.occlusion, 1, frag_in.shellHigh);
}