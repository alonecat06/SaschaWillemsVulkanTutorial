#version 450

//layout (set = 1, binding = 0) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;


layout(push_constant) uniform PushConsts {
	float len;
	float ratio;
	float density;
	float attenuation;
} furFrag;

layout (location = 0) out vec4 outFragColor;

float hash(vec2 uv){
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

float rand(vec2 n) {
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}
float noise(vec2 p){
	vec2 ip = floor(p);
	vec2 u = fract(p);
	u = u*u*(3.0-2.0*u);

	float res = mix(
	mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
	mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
	return res*res;
}

vec4 permute(vec4 x) {
	return mod((34.0 * x + 1.0) * x, 289.0);
}
vec2 cellular2x2(vec2 P) {
	#define K 0.142857142857 // 1/7
	#define K2 0.0714285714285 // K/2
	#define jitter 0.8 // jitter 1.0 makes F1 wrong more often
	vec2 Pi = mod(floor(P), 289.0);
	vec2 Pf = fract(P);
	vec4 Pfx = Pf.x + vec4(-0.5, -1.5, -0.5, -1.5);
	vec4 Pfy = Pf.y + vec4(-0.5, -0.5, -1.5, -1.5);
	vec4 p = permute(Pi.x + vec4(0.0, 1.0, 0.0, 1.0));
	p = permute(p + Pi.y + vec4(0.0, 0.0, 1.0, 1.0));
	vec4 ox = mod(p, 7.0)*K+K2;
	vec4 oy = mod(floor(p*K),7.0)*K+K2;
	vec4 dx = Pfx + jitter*ox;
	vec4 dy = Pfy + jitter*oy;
	vec4 d = dx * dx + dy * dy; // d11, d12, d21 and d22, squared
	// Sort out the two smallest distances
	#if 0
	// Cheat and pick only F1
	d.xy = min(d.xy, d.zw);
	d.x = min(d.x, d.y);
	return d.xx; // F1 duplicated, F2 not computed
	#else
	// Do it right and find both F1 and F2
	d.xy = (d.x < d.y) ? d.xy : d.yx; // Swap if smaller
	d.xz = (d.x < d.z) ? d.xz : d.zx;
	d.xw = (d.x < d.w) ? d.xw : d.wx;
	d.y = min(d.y, d.z);
	d.y = min(d.y, d.w);
	return sqrt(d.xy);
	#endif
}

void main() 
{ 
//	float high = valueNoise(inUV * furFrag.density);
	
//	float high = noise(inUV * furFrag.density);
	
//	vec2 F = cellular2x2(inUV*100);
//	float high = 1.0-1.5*F.x;
	
	float high = hash(floor(inUV * furFrag.density));
	
//	outFragColor = vec4(high, high, high, 1.0);
	
	if (high < furFrag.ratio)
	{
		discard;
	}
	vec4 color = vec4(inColor, 1.0) * pow(furFrag.ratio, furFrag.attenuation);
//	vec3 N = normalize(inNormal);
//	vec3 L = normalize(inLightVec);
//	vec3 V = normalize(inViewVec);
//	vec3 R = reflect(L, N);
//	vec3 diffuse = max(dot(N, L), 0.15) * inColor;
//	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
	outFragColor = vec4(color.rgb, 1.0);
}