#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 48) out;

layout (set = 0, binding = 0) uniform UBOScene
{
    mat4 projection;
    mat4 view;
    vec4 viewPos;
    vec4 lightPos;
} uboScene;

layout (set = 0, binding = 1) uniform UBOFur
{
    float len;
    float viewProdThresh;
    int layers;
} fur;

layout (location = 0) in VS_TO_GS
{
    vec3 normal;
    vec2 uv;
} vertex_in[];

layout (location = 0) out GS_TO_FS
{
    vec3 normal;
    vec2 baseUv;
    vec2 furUv;
    float shellHigh;
    vec3 viewVec;
    vec3 lightVec;
} gemo_out;


void appendFinVertex(vec3 pos, vec3 normal, vec2 baseUv, vec2 finUv, float shellHigh)
{
    gl_Position = uboScene.projection * vec4(pos, 1.0);
    gemo_out.normal = normal;
    gemo_out.baseUv = baseUv;
    gemo_out.furUv = finUv;
    gemo_out.viewVec = uboScene.viewPos.xyz - pos.xyz;
    gemo_out.lightVec = uboScene.lightPos.xyz - pos.xyz;
    gemo_out.shellHigh = shellHigh;

    EmitVertex();
}

void main(void)
{
    // Draw original polygon
    vec3 pos0 = (uboScene.view * gl_in[0].gl_Position).xyz;
    vec3 pos1 = (uboScene.view * gl_in[1].gl_Position).xyz;
    vec3 pos2 = (uboScene.view * gl_in[2].gl_Position).xyz;

    appendFinVertex(pos0, vertex_in[0].normal, vertex_in[0].uv, vec2(-1.0, -1.0), 0);
    appendFinVertex(pos1, vertex_in[1].normal, vertex_in[1].uv, vec2(-1.0, -1.0), 0);
    appendFinVertex(pos2, vertex_in[2].normal, vertex_in[2].uv, vec2(-1.0, -1.0), 0);
    EndPrimitive();
    
    // Draw fin
    vec3 line01 = pos1 - pos0;
    vec3 line02 = pos2 - pos0;
    vec3 normal = normalize(cross(line01, line02));

    vec3 center = (pos0 + pos1 + pos2) / 3;

    vec3 viewDir = normalize(vec3(0, 0, 0) - center);
    float eyeDotN = dot(viewDir, normal);
    if (abs(eyeDotN) > fur.viewProdThresh)
    {
        int i, layer;
        float disp_delta = 1.0 / float(fur.layers);
        float d = 0.0;

        for (layer = 0; layer < fur.layers; layer++)
        {
            for (i = 0; i < gl_in.length(); i++) {
                gemo_out.normal = mat3(uboScene.view) * vertex_in[i].normal;
                vec4 pos = uboScene.view * vec4(gl_in[i].gl_Position.xyz + d * fur.len * vertex_in[i].normal, 1.0);
                gl_Position = uboScene.projection * pos;
                
                gemo_out.baseUv = vertex_in[i].uv * vec2(10, 10);// Unity Shader TRANSFORM_TEX
                gemo_out.furUv = vertex_in[i].uv;

                gemo_out.shellHigh = d;

                gemo_out.viewVec = uboScene.viewPos.xyz - pos.xyz;
                gemo_out.lightVec = uboScene.lightPos.xyz - pos.xyz;

                EmitVertex();
            }
            d += disp_delta;
            EndPrimitive();
        }
    }
    else
    {
        vec2 uvPos0 = vertex_in[0].uv * vec2(10, 10);// Unity Shader TRANSFORM_TEX
        vec3 posFin = pos0 + (line01 + line02) / 2;
        vec2 uvFin = ((vertex_in[1].uv + vertex_in[2].uv) / 2) * vec2(10, 10);// Unity Shader TRANSFORM_TEX

        appendFinVertex(pos0, normal, uvPos0, vec2(0, 1), -1);
        appendFinVertex(posFin, normal, uvFin, vec2(1, 1), -1);
        appendFinVertex(pos0 + fur.len*normal, normal, uvPos0, vec2(0, 0), -1);
        appendFinVertex(posFin + fur.len*normal, normal, uvFin, vec2(1, 0), -1);
        EndPrimitive();

        appendFinVertex(pos0, normal, uvPos0, vec2(0, 1), -1);
        appendFinVertex(pos0 + fur.len*normal, normal, uvPos0, vec2(0, 0), -1);
        appendFinVertex(posFin, normal, uvFin, vec2(1, 1), -1);
        appendFinVertex(posFin + fur.len*normal, normal, uvFin, vec2(1, 0), -1);
        EndPrimitive();
    }
}