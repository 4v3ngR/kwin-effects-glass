#include "roundedcorners.glsl"

uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;

uniform bool noise;
uniform sampler2D noiseTexture;
uniform vec2 noiseTextureSize;

uniform float edgeSizePixels;
uniform float refractionStrength;
uniform float refractionNormalPow;
uniform float refractionRGBFringing;

varying vec2 uv;

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
    vec4 sum = vec4(0, 0, 0, 0);

    if (refractionStrength > 0) {
        vec2 halfBlurSize = 0.49 * blurSize;
        float minHalfSize = min(halfBlurSize.x, halfBlurSize.y);

        vec2 position = uv * blurSize - halfBlurSize.xy;
        float dist = roundedRectangleDist(position, halfBlurSize, topCornerRadius, bottomCornerRadius);

        float edgeFactor = 1.0 - clamp(abs(dist) / edgeSizePixels, 0.0, 1.0);
        float concaveFactor = 1.0 - sqrt(1.0 - pow(edgeFactor, refractionNormalPow));

        // Initial 2D normal
        const float h = 1.0;
        vec2 gradient = vec2(
            roundedRectangleDist(position + vec2(h, 0), halfBlurSize, 1.5 * minHalfSize, 1.5 * minHalfSize) - roundedRectangleDist(position - vec2(h, 0), halfBlurSize, 1.5 * minHalfSize, 1.5 * minHalfSize),
            roundedRectangleDist(position + vec2(0, h), halfBlurSize, 1.5 * minHalfSize, 1.5 * minHalfSize) - roundedRectangleDist(position - vec2(0, h), halfBlurSize, 1.5 * minHalfSize, 1.5 * minHalfSize)
        );

        vec2 normal = length(gradient) > 1e-6 ? -normalize(gradient) : vec2(0.0, 1.0);

        float finalStrength = -0.4 * concaveFactor * refractionStrength;

        vec2 refractOffsetG = normal.xy * finalStrength;
        vec2 refractOffsetR = normal.xy * finalStrength;
        vec2 refractOffsetB = normal.xy * finalStrength;

        // Different refraction offsets for each color channel
        float fringingFactor = refractionRGBFringing * 0.3;
        if (fringingFactor > 0.0) {
            // Red bends most
            refractOffsetR = normal.xy * (finalStrength * (0.8 + fringingFactor));
            // Blue bends least
            refractOffsetB = normal.xy * (finalStrength * (0.8 - fringingFactor));
        }

        vec2 coordR = clamp(uv - refractOffsetR, 0.0, 1.0);
        vec2 coordG = clamp(uv - refractOffsetG, 0.0, 1.0);
        vec2 coordB = clamp(uv - refractOffsetB, 0.0, 1.0);

        sum.r += texture2D(texUnit, coordR).r;
        sum.g += texture2D(texUnit, coordG).g;
        sum.b += texture2D(texUnit, coordB).b;
        sum.a += texture2D(texUnit, coordG).a;
    } else {
        sum += texture2D(texUnit, uv);
    }

    if (noise) {
        sum += vec4(texture2D(noiseTexture, gl_FragCoord.xy / noiseTextureSize).rrr, 0.0);
    }

    gl_FragColor = roundedRectangle(uv * blurSize, sum.rgb);
}