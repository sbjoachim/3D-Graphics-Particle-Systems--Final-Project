// Basic Fragment Shader
// Minimal shader for testing the foundation setup

#version 330

in vec4 vColor;
out vec4 fragColor;

void main(void)
{
	gl_FragColor = vColor;
}
