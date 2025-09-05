#version 450 core

layout(location = 0) in vec4 position; 

out vec3 fragPosition; // フラグメントシェーダに渡す位置データ

uniform float offsetX; // ずらし値
void main() {
    //mat4 mvp2=mvp;
    //mvp2[3][3]=10.;
    //mat4 matrix = mat4(mvp);
    //fragPosition = vec3(position); // 頂点位置をフラグメントシェーダに渡す
    //gl_Position =  vec4(float(mvp[0][1])*vec3(position),1.0);
    gl_Position =  vec4(vec3(position),1.0);
    //gl_Position =  vec4(vec3(position), 1.0);
    //gl_PointSize = 5.0; // 点のサイズ
}