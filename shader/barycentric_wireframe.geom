#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// フラグメントシェーダに渡す重心
out vec3 fragBarycenter;
out vec3 fragBaryCooPoint;

void main(void) {

    mat4 mvp2 = projection * view * model;

    vec3 barycentricCoords[3] = vec3[3](
        vec3(1.0, 0.0, 0.0), // 頂点0
        vec3(0.0, 1.0, 0.0), // 頂点1
        vec3(0.0, 0.0, 1.0)  // 頂点2
    );


    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;

    // 重心を計算
    vec3 barycenter = (p0 + p1 + p2) / 3.0;

    vec3 hen[3]=vec3[3](
    gl_in[1].gl_Position.xyz-gl_in[2].gl_Position.xyz,
    gl_in[2].gl_Position.xyz-gl_in[0].gl_Position.xyz,
    gl_in[0].gl_Position.xyz-gl_in[1].gl_Position.xyz
    );
    float doubled_area=length(cross(hen[0],-hen[2]));

    for (int i = 0; i < 3; i++) {

        gl_Position = mvp2*gl_in[i].gl_Position;
        //gl_Position = gl_in[i].gl_Position;
        fragBarycenter = barycenter;
        barycentricCoords[i]*= doubled_area/length(hen[i]);
        fragBaryCooPoint=barycentricCoords[i];
        EmitVertex();
    }
    EndPrimitive();

}