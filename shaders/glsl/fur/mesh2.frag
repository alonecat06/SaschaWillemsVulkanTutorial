#version 450

//layout (set = 1, binding = 0) uniform sampler2D samplerColorMap;
layout (location = 0) in GS_TO_FS
{
	vec3 normal;
	vec3 color;
	vec2 uv;
	vec3 viewVec;
	vec3 lightVec;
	float fur_strength;
} frag_in;

layout(push_constant) uniform PushConsts {
	float density;
	float attenuation;
} furFrag;

layout (location = 0) out vec4 outFragColor;

float hash(vec2 uv){
	return fract(sin(7.289 * uv.x + 11.23 * uv.y) * 23758.5453);
}

void main() 
{
	float high = hash(floor(frag_in.uv * furFrag.density));
	if (high < frag_in.fur_strength)
	{
		discard;
	}
	vec4 color = vec4(frag_in.color, 1.0) * pow(high, furFrag.attenuation);
	outFragColor = vec4(color.rgb, 1.0);
}