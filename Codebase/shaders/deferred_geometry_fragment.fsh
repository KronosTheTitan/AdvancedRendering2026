#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 fPos;
in vec3 fNor;
in vec2 uv;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{
    // store the fragment position vector in the first gbuffer texture
    gPosition = vec4(fPos,1);
    // also store the per-fragment normals into the gbuffer
    gNormal = vec4(fNor,1);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = vec3(.78f, .78f, .78f);
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 1;
}