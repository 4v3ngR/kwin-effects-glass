#version 140

#include "roundedcorners.glsl"

uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;

uniform bool noise;
uniform sampler2D noiseTexture;
uniform vec2 noiseTextureSize;

uniform float cornerRadius;
uniform float edgeSizePixels;
uniform float refractionStrength;
uniform float refractionNormalPow;
uniform float refractionRGBFringing;

in vec2 uv;
out vec4 fragColor;

// source: https://iquilezles.org/articles/distfunctions2d/
// https://www.shadertoy.com/view/4llXD7
float roundedRectangleDist(vec2 p, vec2 b, float r)
{
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

    if (refractionStrength > 0) {
        vec2 halfBlurSize = 0.5 * blurSize;
        vec2 position = uv * blurSize - halfBlurSize.xy;
        float dist = roundedRectangleDist(position, halfBlurSize, cornerRadius);

        float concaveFactor = pow(clamp(1.0 - abs(dist) / edgeSizePixels, 0.0, 1.0), refractionNormalPow);

        // Initial 2D normal
        const float h = 1.0;
        vec2 gradient = vec2(
            roundedRectangleDist(position + vec2(h, 0), halfBlurSize, edgeSizePixels) - roundedRectangleDist(position - vec2(h, 0), halfBlurSize, edgeSizePixels),
            roundedRectangleDist(position + vec2(0, h), halfBlurSize, edgeSizePixels) - roundedRectangleDist(position - vec2(0, h), halfBlurSize, edgeSizePixels)
        );

        vec2 normal = length(gradient) > 1e-6 ? -normalize(gradient) : vec2(0.0, 1.0);

        float finalStrength = -0.4 * concaveFactor * refractionStrength;

        // Different refraction offsets for each color channel
        float fringingFactor = refractionRGBFringing * 0.3;
        vec2 refractOffsetR = normal.xy * (finalStrength * (0.8 + fringingFactor)); // Red bends most
        vec2 refractOffsetG = normal.xy * finalStrength;
        vec2 refractOffsetB = normal.xy * (finalStrength * (0.8 - fringingFactor)); // Blue bends least

        vec2 coordR = clamp(uv - refractOffsetR, 0.0, 1.0);
        vec2 coordG = clamp(uv - refractOffsetG, 0.0, 1.0);
        vec2 coordB = clamp(uv - refractOffsetB, 0.0, 1.0);

        for (int i = 0; i < 8; ++i) {
            vec2 off = offsets[i] * offset;
            sum.r += texture(texUnit, coordR + off).r * weights[i];
            sum.g += texture(texUnit, coordG + off).g * weights[i];
            sum.b += texture(texUnit, coordB + off).b * weights[i];
            sum.a += texture(texUnit, coordG + off).a * weights[i];
        }

        sum /= weightSum;
    } else {
        for (int i = 0; i < 8; ++i) {
            vec2 off = offsets[i] * offset;
            sum += texture(texUnit, uv + off)  * weights[i];
        }

        sum /= weightSum;
    }

    if (noise) {
        sum += vec4(texture(noiseTexture, gl_FragCoord.xy / noiseTextureSize).rrr, 0.0);
    }

    fragColor = roundedRectangle(uv * blurSize, sum.rgb);
}