#version 330 core

out vec4 FragColor;
in vec3 vColours;

void main()
{
	FragColor = vec4(vColours, 1.0);
}