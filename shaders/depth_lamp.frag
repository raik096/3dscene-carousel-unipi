#version 460 core
in vec4 wPos;

uniform vec3 uLightPos;
uniform float uFarPlane;

uniform sampler2D uTexture;

void main()
{
	uTexture;
    
    // get distance between fragment and light source
    float lightDistance = length(wPos.xyz - uLightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / uFarPlane;
    
    // write this as modified depth
    gl_FragDepth = 0;
}  