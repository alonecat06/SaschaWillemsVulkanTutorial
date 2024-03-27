#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 9) out;

layout (set = 0, binding = 0) uniform UBOScene
{
    mat4 projection;
    mat4 view;
    vec4 lightPos;
    vec4 viewPos;
} ubo;

layout (set = 0, binding = 1) uniform UBOFur
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

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;

void main(void)
{
    float normalLength = 0.2;
    for(int i=0; i<gl_in.length(); i++)
    {
        vec3 pos = gl_in[i].gl_Position.xyz;
        vec3 normal = vertex_in[i].normal;
        outNormal = normal;

        gl_Position = ubo.projection * (ubo.view * vec4(pos + (normal+vec3(-1, 0, 0)) * normalLength, 1.0));
        outColor = vec3(1.0, 0.0, 0.0);
        EmitVertex();

        gl_Position = ubo.projection * (ubo.view * vec4(pos + (normal+vec3(1, 0, 0)) * normalLength, 1.0));
        outColor = vec3(0.0, 0.0, 1.0);
        EmitVertex();

        gl_Position = ubo.projection * (ubo.view * vec4(pos + (normal+vec3(0, 1, 0)) * normalLength, 1.0));
        outColor = vec3(0.0, 1.0, 0.0);
        EmitVertex();

        EndPrimitive();
    }
}