#version 440 core

//A quick test shader for glMultiDrawElementsIndirect
//two attributes are expected, position and color

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

out vec3 fcolor;

void main(void){
	fcolor = color;
	gl_Position = vec4(pos, 1);
}

