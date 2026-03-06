#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
in vec3 vModelPos;
in vec2 vUV;
noperspective in vec2 vUV_Affine;
out vec4 FragColor;
uniform sampler2D uTex;
uniform float uTileSize;
uniform vec3 u_LightPos;
uniform float u_LightStrength;
uniform bool u_UseAffine;
void main() {
    // Derive geometric normal from screen-space derivatives of the world position.
    // This is immune to the model's rotation baking normals into the wrong axis.
    vec3  N = normalize(cross(dFdx(vFragPos), dFdy(vFragPos)));
    if (dot(N, vec3(0.0, 1.0, 0.0)) < 0.0) N = -N;  // always face upward

    vec4 texColor;
    if (u_UseAffine) {
        // Affine (PS1-style): linear UV interpolation, no perspective correction.
        texColor = texture(uTex, fract(vUV_Affine * uTileSize));
    } else {
        // Triplanar mapping: project texture from all 3 world-space axes, blend by normal.
        // Fixes stretching on meshes whose model-space orientation doesn't align with XZ.
        vec3 blend = abs(N);
        blend = max(blend, 0.001);
        blend /= (blend.x + blend.y + blend.z);
        vec4 tX = texture(uTex, fract(vFragPos.yz * uTileSize));
        vec4 tY = texture(uTex, fract(vFragPos.xz * uTileSize));
        vec4 tZ = texture(uTex, fract(vFragPos.xy * uTileSize));
        texColor = tX * blend.x + tY * blend.y + tZ * blend.z;
    }

    vec3  lightDir    = normalize(u_LightPos - vFragPos);
    float dist        = length(u_LightPos - vFragPos);
    float attenuation = 1.0 / (1.0 + 0.14 * dist + 0.07 * dist * dist);
    float diff        = max(dot(N, lightDir), 0.0);

    vec3 ambient  = texColor.rgb * 0.15;
    vec3 diffuse  = texColor.rgb * diff * attenuation * u_LightStrength;

    FragColor = vec4(clamp(ambient + diffuse, 0.0, 1.0), 1.0);
}
