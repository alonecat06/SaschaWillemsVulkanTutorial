#version 450
layout (set = 0, binding = 2) uniform sampler2D samplerBaseColor;
layout (set = 0, binding = 3) uniform sampler2D samplerFinMap;
layout (set = 0, binding = 3) uniform sampler2D samplerNoiseMap;

layout (location = 0) in GS_TO_FS
{
	vec3 normal;
	vec2 baseUv;
	vec2 finUv;
	float shellHigh;
	vec3 viewVec;
	vec3 lightVec;
	float lodBias;
} frag_in;

layout(push_constant) uniform PushConsts {
	float alphaCutout;
	float occlusion;
	float density;
	float attenuation;
	float thickness;
} furFrag;

layout (location = 0) out vec4 outFragColor;

float hash(vec2 uv){
	return fract(sin(7.289 * uv.x + 11.23 * uv.y) * 23758.5453);
}

void main()
{
	if (frag_in.shellHigh >= 0)
	{
		vec2 localspace = fract(frag_in.baseUv * furFrag.density) * 2 - 1;
		float dist = length(localspace);
		float rnd_high = hash(floor(frag_in.baseUv * furFrag.density));

		if (frag_in.shellHigh > 0 && dist > furFrag.thickness * (rnd_high / frag_in.shellHigh - 1))
		{
			discard;
		}

//		vec4 color = vec4(frag_in.color, 1.0) * pow(frag_in.shellHigh, furFrag.attenuation);
//		outFragColor = vec4(color.rgb, 1.0);
		outFragColor = texture(samplerBaseColor, frag_in.baseUv, frag_in.lodBias);
		outFragColor.rgb *= mix(1, 1 - furFrag.occlusion, frag_in.finUv.y);
	}
	else
	{
		vec4 furColor = texture(samplerFinMap, frag_in.finUv, frag_in.lodBias);
		if (frag_in.finUv.x > 0 && furColor.a < furFrag.alphaCutout)
		{
			discard;
		}

		outFragColor = texture(samplerBaseColor, frag_in.baseUv, frag_in.lodBias);
		outFragColor.rgb *= mix(1, 1 - furFrag.occlusion, frag_in.finUv.y);
	}
}