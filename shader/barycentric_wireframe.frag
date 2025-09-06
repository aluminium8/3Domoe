#version 450 core
out vec4 fragColor;


in vec3 fragBarycenter;
in vec3 fragBaryCooPoint;
void main() {
    float is_inside = step(0.005, min(fragBaryCooPoint.x,min(fragBaryCooPoint.y,fragBaryCooPoint.z)));
    fragColor = mix(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), is_inside);
}