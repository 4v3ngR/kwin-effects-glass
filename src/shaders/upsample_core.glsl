#version 140

#include "roundedcorners.glsl"

uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;

uniform bool noise;
uniform sampler2D noiseTexture;
uniform vec2 noiseTextureSize;

uniform vec3 tintColor;
uniform float tintStrength;

uniform vec3 glowColor;
uniform float glowStrength;
uniform int edgeLighting;

uniform float edgeSizePixels;
uniform float refractionStrength;
uniform float refractionNormalPow;
uniform float refractionRGBFringing;

in vec2 uv;
out vec4 fragColor;

// source: https://iquilezles.org/articles/distfunctions2d/
// https://www.shadertoy.com/view/4llXD7
float roundedRectangleDist(vec2 p, vec2 b, float topRadius, float bottomRadius)
{
    float r = (p.y > 0.0) ? topRadius : bottomRadius;
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main(void)
{
    vec2 offsets[8] = vec2[](
        vec2(-halfpixel.x * 2.0, 0.0),
        vec2(-halfpixel.x, halfpixel.y),
        vec2(0.0, halfpixel.y * 2.0),
        vec2(halfpixel.x, halfpixel.y),
        vec2(halfpixel.x * 2.0, 0.0),
        vec2(halfpixel.x, -halfpixel.y),
        vec2(0.0, -halfpixel.y * 2.0),
        vec2(-halfpixel.x, -halfpixel.y)
    );
    float weights[8] = float[](1.0, 2.0, 1.0, 2.0, 1.0, 2.0, 1.0, 2.0);
    float weightSum = 12.0;

    vec4 sum = vec4(0, 0, 0, 0);
    vec2 halfBlurSize = 0.5 * blurSize;
    float minHalfSize = min(halfBlurSize.x, halfBlurSize.y);

    if (refractionStrength > 0 && minHalfSize >= 16.0) {

        vec2 position = uv * blurSize - halfBlurSize.xy;
        float dist = roundedRectangleDist(position, halfBlurSize, topCornerRadius, bottomCornerRadius);

        if (dist > 0.0) {
            fragColor = roundedRectangle(uv * blurSize, texture(texUnit, uv).rgb);
            return;
        }

        float edgeFactor = 1.0 - clamp(abs(dist) / edgeSizePixels, 0.0, 1.0);
        float concaveFactor = 1.0 - sqrt(1.0 - pow(edgeFactor, refractionNormalPow));

        // Initial 2D normal
        const float h = 1.0;
        vec2 gradient = vec2(
            roundedRectangleDist(position + vec2(h, 0), halfBlurSize,  minHalfSize, minHalfSize) - roundedRectangleDist(position - vec2(h, 0), halfBlurSize, minHalfSize, minHalfSize),
            roundedRectangleDist(position + vec2(0, h), halfBlurSize, minHalfSize, minHalfSize) - roundedRectangleDist(position - vec2(0, h), halfBlurSize, minHalfSize, minHalfSize)
        );

        vec2 normal = length(gradient) > 0.0 ? -normalize(gradient) : vec2(0.0, 1.0);

        float finalStrength = min(0.4 * concaveFactor * refractionStrength, 0.4);

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

        float blurRadius = 1.2 * (1.0 - edgeFactor * 0.5);
        for (int i = 0; i < 8; i++) {
            vec2 off = offsets[i] * blurRadius;
            float weight = weights[i];
            sum.r += texture(texUnit, coordR + off).r * weight;
            sum.g += texture(texUnit, coordG + off).g * weight;
            sum.b += texture(texUnit, coordB + off).b * weight;
            sum.a += texture(texUnit, coordG + off).a * weight;
        }
        sum /= weightSum;

        if (concaveFactor < 1.0) {
            vec3 glow = mix(sum.rgb, glowColor, clamp(0.25 * concaveFactor, 0.0, glowStrength));
            if (edgeLighting == 1) {
                glow += (sum.rgb * concaveFactor);
            }

            sum.r = glow.r;
            sum.g = glow.g;
            sum.b = glow.b;
        }
    } else {
        for (int i = 0; i < 8; ++i) {
            vec2 off = offsets[i] * offset;
            sum += texture2D(texUnit, uv + off) * weights[i];
        }

        sum /= weightSum;
    }

    if (noise) {
        sum += vec4(texture2D(noiseTexture, vec2(uv.x, 1.0 - uv.y) * blurSize / noiseTextureSize).rrr, 0.0);
        // sum += vec4(texture(noiseTexture, gl_FragCoord.xy / noiseTextureSize).rrr, 0.0);
    }

    vec3 tinted = mix(sum.rgb, tintColor, clamp(tintStrength, 0.0, 1.0));
    fragColor = roundedRectangle(uv * blurSize, tinted);
}