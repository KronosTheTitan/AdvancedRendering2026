#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aUv;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 fPos;
out vec3 fNor;
out vec2 uv;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    fPos = worldPos.xyz;

    uv = aUv;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    fNor = normalMatrix * aNor;
    gl_Position = projection * view * worldPos;
}