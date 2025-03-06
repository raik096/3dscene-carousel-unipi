#version 460 core 
layout (location = 0) in vec3 aPosition; 

uniform mat4 uModel;
uniform mat4 uLightMatrix;


void main(void) 
{ 
    gl_Position = uLightMatrix*uModel*vec4(aPosition, 1.0); 
}