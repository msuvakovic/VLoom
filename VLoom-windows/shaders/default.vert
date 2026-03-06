#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
out vec3 vNormal;
out vec3 vFragPos;
out vec3 vModelPos;
out vec2 vUV;
noperspective out vec2 vUV_Affine;
uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Model;
void main() {
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);
    vFragPos    = vec3(u_Model * vec4(aPos, 1.0));
    vNormal     = mat3(transpose(inverse(u_Model))) * aNormal;
    vModelPos   = aPos;
    vUV         = aUV;
    vUV_Affine  = aUV;
}
