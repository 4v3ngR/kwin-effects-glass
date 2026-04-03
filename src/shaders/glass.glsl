uniform vec3 tintColor;
uniform float tintStrength;
uniform vec3 glowColor;
uniform float glowStrength;
uniform int edgeLighting;

uniform float edgeSizePixels;
uniform float refractionStrength;
uniform float refractionNormalPow;
uniform float refractionRGBFringing;

uniform float saturationBoost;

// --- Oklab color space conversion ---
vec3 srgbToLinear(vec3 c)
{
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

vec3 linearToSrgb(vec3 c)
{
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

vec3 linearToOklab(vec3 c)
{
    float l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;
    float m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;
    float s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;

    float l_ = pow(max(l, 0.0), 1.0 / 3.0);
    float m_ = pow(max(m, 0.0), 1.0 / 3.0);
    float s_ = pow(max(s, 0.0), 1.0 / 3.0);

    return vec3(
        0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
        1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
        0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_
    );
}

vec3 oklabToLinear(vec3 lab)
{
    float l_ = lab.x + 0.3963377774 * lab.y + 0.2158037573 * lab.z;
    float m_ = lab.x - 0.1055613458 * lab.y - 0.0638541728 * lab.z;
    float s_ = lab.x - 0.0894841775 * lab.y - 1.2914855480 * lab.z;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return vec3(
         4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
        -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
        -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    );
}

vec3 oklabSatBoost(vec3 color, float amount)
{
    if (abs(amount - 1.0) < 0.001) return color;

    vec3 linear = srgbToLinear(clamp(color, 0.0, 1.0));
    vec3 lab = linearToOklab(linear);
    lab.yz *= amount;
    vec3 result = oklabToLinear(lab);
    return linearToSrgb(clamp(result, 0.0, 1.0));
}

float roundedRectangleDist(vec2 p, vec2 b, vec4 cornerRadius)
{
    float r = p.x > 0.0
        ? (p.y > 0.0 ? cornerRadius.y : cornerRadius.w)
        : (p.y > 0.0 ? cornerRadius.x : cornerRadius.z);
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

vec4 glass(vec4 sum, vec4 cornerRadius)
{
    vec2 halfBlurSize = blurSize * 0.5;
    vec2 position = uv * blurSize - halfBlurSize;

    // --- Inline SDF + analytical gradient (one SDF, reused everywhere) ---
    float cr = roundedRectangleDist(position, halfBlurSize, cornerRadius);
    vec2 q = abs(position) - halfBlurSize + cr;
    float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - cr;

    if (dist >= 0.0) {
        return sum;
    }

    float ior = 1.0 + refractionStrength;

    if (ior > 1.001) {
        // Analytical SDF gradient
        vec2 s = sign(position);
        vec2 gradQ;
        if (q.x > 0.0 && q.y > 0.0) {
            gradQ = normalize(q);
        } else if (q.x > 0.0) {
            gradQ = vec2(1.0, 0.0);
        } else if (q.y > 0.0) {
            gradQ = vec2(0.0, 1.0);
        } else {
            gradQ = q.x > q.y ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
        }
        vec2 sdfGrad = gradQ * s;

        // --- Analytical glass normal ---
        float invEdge = 1.0 / max(edgeSizePixels, 0.1);
        float t = clamp(-dist * invEdge, 0.0, 1.0);
        float dh = refractionNormalPow * pow(1.0 - t, refractionNormalPow - 1.0) * 0.15;
        vec3 N = normalize(vec3(dh * sdfGrad, 1.0));

        vec3 I = vec3(0.0, 0.0, -1.0);
        float thickness = edgeSizePixels;

        // Chromatic dispersion
        float dispersion = refractionRGBFringing * 0.04;
        float ior_r = ior - dispersion;
        float ior_b = ior + dispersion;

        vec3 R_r = refract(I, N, 1.0 / max(ior_r, 1.001));
        vec3 R_g = refract(I, N, 1.0 / max(ior, 1.001));
        vec3 R_b = refract(I, N, 1.0 / max(ior_b, 1.001));

        // handle total internal reflection 
        if (dot(R_r, R_r) < 0.000001) R_r = vec3(0.0, 0.0, -1.0);
        if (dot(R_g, R_g) < 0.000001) R_g = vec3(0.0, 0.0, -1.0);
        if (dot(R_b, R_b) < 0.000001) R_b = vec3(0.0, 0.0, -1.0);

        // --- SDF-driven lens distortion ---
        float edgeFactor = 1.0 - clamp(-dist * invEdge, 0.0, 1.0);
        float lensMag = edgeFactor * edgeSizePixels;

        // Edge UV bending
        vec2 normalizedPos = position / halfBlurSize;
        sdfGrad += normalizedPos * edgeFactor;

        vec2 uvScale = 1.0 / blurSize;
        vec2 lensOffset = -sdfGrad * lensMag * uvScale;

        vec2 offset_r = -R_r.xy / abs(R_r.z) * thickness * uvScale + lensOffset;
        vec2 offset_g = -R_g.xy / abs(R_g.z) * thickness * uvScale + lensOffset;
        vec2 offset_b = -R_b.xy / abs(R_b.z) * thickness * uvScale + lensOffset;

        vec4 sampleG = TEXTURE(texUnit, clamp(uv + offset_g, 0.0, 1.0));
        sum.r = TEXTURE(texUnit, clamp(uv + offset_r, 0.0, 1.0)).r;
        sum.g = sampleG.g;
        sum.b = TEXTURE(texUnit, clamp(uv + offset_b, 0.0, 1.0)).b;
        sum.a = sampleG.a;

        // Fresnel
        float cosTheta = N.z;
        float F0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
        float oneMinusCos = 1.0 - cosTheta;
        float oneMinusCos2 = oneMinusCos * oneMinusCos;
        float fresnel = F0 + (1.0 - F0) * oneMinusCos2 * oneMinusCos2 * oneMinusCos;

        sum.rgb = mix(sum.rgb, glowColor, fresnel * glowStrength);

        if (edgeLighting == 1) {
            sum.rgb += sum.rgb * fresnel * 0.5;
        }

        if (glowStrength > 0.0) {
            float edgeMask = smoothstep(0.0, -2.0, dist); 
            float borderInner = smoothstep(-1.0, -3.0, dist);
            float edgeProfile = edgeMask - borderInner; 
            float thicknessShadow = pow(edgeProfile, 0.9);
            float shadowMask = smoothstep(blurSize.y * 0.7 , -blurSize.y * 0.7 , position.y) *
                               smoothstep(blurSize.x * 0.7 , -blurSize.x * 0.7 , -position.x);
            float highlightMask = smoothstep(-blurSize.y * 0.7 , blurSize.y * 0.7 , position.y) *
                                  smoothstep(-blurSize.x * 0.7 , blurSize.x * 0.7 , -position.x);

            glow = mix(glow, vec3(1.0), thicknessShadow * shadowMask);
            glow = mix(glow, vec3(1.0), thicknessShadow * highlightMask);
        }


        sum.r = glow.r;
        sum.g = glow.g;
        sum.b = glow.b;
    }

    if (abs(saturationBoost - 1.0) > 0.001) {
        sum.rgb = oklabSatBoost(sum.rgb, saturationBoost);
    }

    vec3 tinted = mix(sum.rgb, tintColor, clamp(tintStrength, 0.0, 1.0));

    // dist < 0 guaranteed here; outer sdfRoundedBox handles edge AA
    return vec4(tinted, 1.0);
}
