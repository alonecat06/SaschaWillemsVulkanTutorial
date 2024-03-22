#version 450

//layout (set = 1, binding = 0) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;
layout (location = 5) in float inLayerRatio;

layout (location = 0) out vec4 outFragColor;

float hash(vec2 uv)
{
	return fract(sin(7.289 * uv.x + 11.23 * uv.y) * 23758.5453);
}

float valueNoise(vec2 uv) {
	vec2 u = floor(uv);
	vec2 f = fract(uv);
	float a = hash(u);
	float b = hash(u + vec2(1.0, 0.0));
	float c = hash(u + vec2(0.0, 1.0));
	float d = hash(u + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x),
	mix(c, d, f.x), f.y);
}

void main() 
{
	if (hash(inUV * 1000.0) < inLayerRatio)
	{
//		outFragColor = vec4(0.0, 0.0, 0.0, 1.0);
//		return;
		discard;
	}
	
	vec4 color = vec4(inColor, 1.0) * inLayerRatio;

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(L, N);
	vec3 diffuse = max(dot(N, L), 0.15) * inColor;
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
	outFragColor = vec4(diffuse * color.rgb + specular, 1.0);
}