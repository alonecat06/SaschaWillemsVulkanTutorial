#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 240) out;

layout (set = 0, binding = 0) uniform UBOScene
{
    mat4 projection;
    mat4 view;
    vec4 lightPos;
    vec4 viewPos;
} uboScene;

layout (set = 1, binding = 0) uniform UBOFur
{
    float len;
    int layers;
} fur;

layout (location = 0) in VS_TO_GS
{
    vec3 normal;
    vec3 color;
    vec2 uv;
} vertex_in[];

layout (location = 0) out GS_TO_FS
{
    vec3 normal;
    vec3 color;
    vec2 uv;
    vec3 viewVec;
    vec3 lightVec;
    float fur_strength;
} gemo_out;

void main(void)
{
    int i, layer;
    float disp_delta = 1.0 / float(fur.layers);
    float d = 0.0;

    for (layer = 0; layer < fur.layers; layer++)
    {
        for (i = 0; i < gl_in.length(); i++) {
            vec4 pos = uboScene.view * (gl_in[i].gl_Position + vec4(vertex_in[i].normal * d * fur.len, 1.0));
            gl_Position = uboScene.projection * pos;
            
            gemo_out.normal = mat3(uboScene.view) * vertex_in[i].normal;
            gemo_out.color = vertex_in[i].color;
            gemo_out.uv = vertex_in[i].uv;

            gemo_out.viewVec = uboScene.viewPos.xyz - pos.xyz;
            gemo_out.lightVec = uboScene.lightPos.xyz - pos.xyz;

            gemo_out.fur_strength = d;
            
            EmitVertex();
        }
        d += disp_delta;
        EndPrimitive();
    }
}