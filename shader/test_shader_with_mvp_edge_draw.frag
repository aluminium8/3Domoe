#version 450 core
out vec4 fragColor;


in vec3 fragBarycenter;
in vec3 fragBaryCooPoint;
void main() {
    fragColor =mix(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), min(fragBaryCooPoint.x,min(fragBaryCooPoint.y,fragBaryCooPoint.z)) > 0.005);
    //fragColor= vec4(1.0, 0.5, 0.2, 1.0);
}