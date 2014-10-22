#version 440 core

in vec3 fcolor;

out vec4 color;

void main(void){
	color = vec4(fcolor, 1);
}

