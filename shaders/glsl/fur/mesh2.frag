#version 450

//layout (set = 1, binding = 0) uniform sampler2D samplerColorMap;
layout (location = 0) in GS_TO_FS
{
	vec3 normal;
	vec3 color;
	vec2 uv;
	vec3 viewVec;
	vec3 lightVec;
	float shell_high;
} frag_in;

layout(push_constant) uniform PushConsts {
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
	vec2 localspace = fract(frag_in.uv * furFrag.density) * 2 - 1;
	float dist = length(localspace);
	float rnd_high = hash(floor(frag_in.uv * furFrag.density));
	
//	if (dist > furFrag.thickness)
//	{
//		discard;
//	}
//	if (rnd_high < frag_in.shell_high)
//	{
//		discard;
//	}

//	if (dist > furFrag.thickness * (rnd_high - frag_in.shell_high))
//	{
//		discard;
//	}

	if (frag_in.shell_high > 0 && dist > furFrag.thickness * (rnd_high / frag_in.shell_high - 1))
	{
		discard;
		//outFragColor = vec4(dist,furFrag.thickness * (rnd_high / frag_in.shell_high - 1),rnd_high, frag_in.shell_high);
	}

	vec4 color = vec4(frag_in.color, 1.0) * pow(frag_in.shell_high, furFrag.attenuation);
	outFragColor = vec4(color.rgb, 1.0);
}