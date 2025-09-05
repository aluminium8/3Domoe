#version 450 core
out vec4 FragColor;

in vec3 FragNormal;

void main()
{
    FragColor = vec4(FragNormal * 0.5 + 0.5, 1.0);
}
