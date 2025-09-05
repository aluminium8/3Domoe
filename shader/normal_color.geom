#version 450 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform dmat4 mvp;

out vec3 FragNormal;

void main()
{
    vec3 v0 = gl_in[0].gl_Position.xyz;
    vec3 v1 = gl_in[1].gl_Position.xyz;
    vec3 v2 = gl_in[2].gl_Position.xyz;

    vec3 faceNormal = normalize(cross(v1 - v0, v2 - v0));

    mat4 mvp_float = mat4(mvp);

    gl_Position = mvp_float * gl_in[0].gl_Position;
    FragNormal = faceNormal;
    EmitVertex();

    gl_Position = mvp_float * gl_in[1].gl_Position;
    FragNormal = faceNormal;
    EmitVertex();

    gl_Position = mvp_float * gl_in[2].gl_Position;
    FragNormal = faceNormal;
    EmitVertex();

    EndPrimitive();
}
