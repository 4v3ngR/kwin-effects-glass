uniform vec3 tintColor;
uniform float tintStrength;
uniform vec3 glowColor;
uniform float glowStrength;
uniform int edgeLighting;

uniform float edgeSizePixels;
uniform float refractionStrength;
uniform float refractionNormalPow;
uniform float refractionRGBFringing;

float roundedRectangleDist(vec2 p, vec2 b, vec4 cornerRadius)
{
    float r = p.x > 0.0
        ? (p.y < 0.0 ? cornerRadius.y : cornerRadius.w)
        : (p.y < 0.0 ? cornerRadius.x : cornerRadius.z);
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

vec4 roundedRectangle(vec2 fragCoord, vec3 texture, vec4 cornerRadius)
{
    vec2 halfblurSize = blurSize * 0.5;
    vec2 p = fragCoord - halfblurSize;
    float dist = roundedRectangleDist(p, halfblurSize, cornerRadius);

    if (dist <= 0.0) {
        return vec4(texture, 1.0);
    }

    float s = smoothstep(0.0, 1.0,  dist);
    return vec4(texture, mix(1.0, 0.0, s));
}

vec4 glass(vec4 sum, vec4 cornerRadius)
{
    vec2 halfBlurSize = blurSize * 0.5;
    float minHalfSize = min(halfBlurSize.x, halfBlurSize.y);

    vec2 position = uv * blurSize - halfBlurSize.xy;
    float dist = roundedRectangleDist(position, halfBlurSize, cornerRadius);

    if (dist >= 0.0) {
        return sum;
    }

    float minEsp = clamp(edgeSizePixels, 0.1, minHalfSize * 0.9);
    float edgeFactor = 1.0 - clamp(abs(dist) / minEsp, 0.0, 1.0);
    float concaveFactor = 1.0 - sqrt(1.0 - pow(smoothstep(0.0, 1.0, edgeFactor), refractionNormalPow));

    if (refractionStrength > 0 && minHalfSize >= 16.0) {
        // Initial 2D normal
        const float h = 1.0;
        vec4 r = clamp(cornerRadius * 2.0, 64.0, 128.0);
        vec2 gradient = vec2(
                roundedRectangleDist(position + vec2(h, 0), halfBlurSize, r) - roundedRectangleDist(position - vec2(h, 0), halfBlurSize, r),
                roundedRectangleDist(position + vec2(0, h), halfBlurSize, r) - roundedRectangleDist(position - vec2(0, h), halfBlurSize, r)
        );

        vec2 normal = length(gradient) > 0.0 ? -normalize(gradient) : vec2(0.0, 1.0);

        float finalStrength = min(0.4 * concaveFactor * refractionStrength, 1.0);

        vec2 refractOffsetG = -normal.xy * finalStrength;
        vec2 refractOffsetR = -normal.xy * finalStrength;
        vec2 refractOffsetB = -normal.xy * finalStrength;

        // Different refraction offsets for each color channel
        float fringingFactor = refractionRGBFringing * 0.3;
        if (fringingFactor > 0.0) {
            // Red bends most
            refractOffsetR = -normal.xy * (finalStrength * (1.0 + fringingFactor));
            // Blue bends least
            refractOffsetB = -normal.xy * (finalStrength * (1.0 - fringingFactor));
        }

        vec2 coordR = clamp(uv - refractOffsetR, 0.0, 1.0);
        vec2 coordG = clamp(uv - refractOffsetG, 0.0, 1.0);
        vec2 coordB = clamp(uv - refractOffsetB, 0.0, 1.0);

        sum.r = texture(texUnit, coordR).r;
        sum.g = texture(texUnit, coordG).g;
        sum.b = texture(texUnit, coordB).b;
        sum.a = texture(texUnit, coordG).a;
    }

    if (concaveFactor < 1.0) {
        vec3 glow = mix(sum.rgb, glowColor, clamp(0.25 * concaveFactor, 0.0, glowStrength));
        if (edgeLighting == 1) {
            glow += (sum.rgb * concaveFactor);
        }

        sum.r = glow.r;
        sum.g = glow.g;
        sum.b = glow.b;
    }

    vec3 tinted = mix(sum.rgb, tintColor, clamp(tintStrength, 0.0, 1.0));
    return roundedRectangle(uv * blurSize, tinted, cornerRadius);
}
