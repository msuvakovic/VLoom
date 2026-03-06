#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
in vec3 vModelPos;
in vec2 vUV;
noperspective in vec2 vUV_Affine;
out vec4 FragColor;
uniform sampler2D uTex;
uniform sampler2D uNormalMap;
uniform float uTileSize;
uniform vec3 u_LightPos;
uniform float u_LightStrength;
uniform int u_UseAffine;
uniform int u_UseNormalMap;
uniform mat4 u_Model;
void main() {
    // gl_FrontFacing flips vNormal on back faces so it always points outward,
    // regardless of winding order in the imported FBX assets.
    vec3 geomN = normalize(vNormal) * (gl_FrontFacing ? 1.0 : -1.0);

    vec3 N;
    vec4 texColor;

    if (u_UseAffine != 0) {
        // Affine (PS1-style): linear UV interpolation, no perspective correction.
        texColor = texture(uTex, fract(vUV_Affine * uTileSize));
        N = geomN;
    } else {
        // Triplanar mapping: project texture from all 3 model-space axes, blend by normal.
        // Uses vModelPos so the texture is locked to the mesh.
        // Blend weights derived from model-space geometric normal (dFdx/dFdy of vModelPos)
        // so they match the model-space UV coords regardless of world orientation.
        vec3 mN = normalize(cross(dFdx(vModelPos), dFdy(vModelPos)));
        vec3 blend = abs(mN);
        blend = max(blend, 0.001);
        blend /= (blend.x + blend.y + blend.z);
        vec4 tX = texture(uTex, fract(vModelPos.yz * uTileSize));
        vec4 tY = texture(uTex, fract(vModelPos.xz * uTileSize));
        vec4 tZ = texture(uTex, fract(vModelPos.xy * uTileSize));
        texColor = tX * blend.x + tY * blend.y + tZ * blend.z;

        if (u_UseNormalMap != 0) {
            // Triplanar normal mapping — sample the normal map from each axis projection,
            // then remap the tangent-space result to model space using the analytical TBN
            // for that projection. No per-vertex tangents needed.
            //
            // Per-axis TBN (columns = T, B, N in model space):
            //   X-proj (UV=.yz): T=+Y, B=+Z, N=±X  →  model = (b*sign, r, g)
            //   Y-proj (UV=.xz): T=+X, B=+Z, N=±Y  →  model = (r, b*sign, g)
            //   Z-proj (UV=.xy): T=+X, B=+Y, N=±Z  →  model = (r, g, b*sign)
            vec3 tnX = texture(uNormalMap, fract(vModelPos.yz * uTileSize)).rgb * 2.0 - 1.0;
            vec3 tnY = texture(uNormalMap, fract(vModelPos.xz * uTileSize)).rgb * 2.0 - 1.0;
            vec3 tnZ = texture(uNormalMap, fract(vModelPos.xy * uTileSize)).rgb * 2.0 - 1.0;

            vec3 nX = vec3(tnX.z * sign(mN.x), tnX.x, tnX.y);
            vec3 nY = vec3(tnY.x, tnY.z * sign(mN.y), tnY.y);
            vec3 nZ = vec3(tnZ.x, tnZ.y, tnZ.z * sign(mN.z));

            vec3 blendedModelN = normalize(nX * blend.x + nY * blend.y + nZ * blend.z);
            mat3 normalMat = mat3(transpose(inverse(u_Model)));
            N = normalize(normalMat * blendedModelN);
            // Ensure N faces the same outward direction as the (front-face-corrected) geomN.
            if (dot(N, geomN) < 0.0) N = -N;
        } else {
            N = geomN;
        }
    }

    vec3  lightDir    = normalize(u_LightPos - vFragPos);
    float dist        = length(u_LightPos - vFragPos);
    float attenuation = 1.0 / (1.0 + 0.14 * dist + 0.07 * dist * dist);
    float diff        = max(dot(N, lightDir), 0.0);

    vec3 ambient  = texColor.rgb * 0.15;
    vec3 diffuse  = texColor.rgb * diff * attenuation * u_LightStrength;

    FragColor = vec4(clamp(ambient + diffuse, 0.0, 1.0), 1.0);
}
