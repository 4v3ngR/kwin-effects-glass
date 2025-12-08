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
        vec2 halfBlurSize = 0.49 * blurSize;
        float minHalfSize = min(halfBlurSize.x, halfBlurSize.y);

        vec2 position = uv * blurSize - halfBlurSize.xy;
        float dist = roundedRectangleDist(position, halfBlurSize, topCornerRadius, bottomCornerRadius);
        float normDist = dist / minHalfSize;
        float normEdgeSize = edgeSizePixels / minHalfSize;
        float edgeFactor = clamp(1.0 - abs(normDist) / normEdgeSize, 0.0, 1.0);
        float concaveFactor = pow(edgeFactor, refractionNormalPow);

        // Initial 2D normal
        const float h = 1.0;
        vec2 gradient = vec2(
            roundedRectangleDist(position + vec2(h, 0), halfBlurSize, minHalfSize, minHalfSize) - roundedRectangleDist(position - vec2(h, 0), halfBlurSize, minHalfSize, minHalfSize),
            roundedRectangleDist(position + vec2(0, h), halfBlurSize, minHalfSize, minHalfSize) - roundedRectangleDist(position - vec2(0, h), halfBlurSize, minHalfSize, minHalfSize)
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

        for (int i = 0; i < 8; ++i) {
            vec2 off = offsets[i] * offset;
            sum.r += texture2D(texUnit, coordR + off).r * weights[i];
            sum.g += texture2D(texUnit, coordG + off).g * weights[i];
            sum.b += texture2D(texUnit, coordB + off).b * weights[i];
            sum.a += texture2D(texUnit, coordG + off).a * weights[i];
        }

        sum /= weightSum;
    } else {
        for (int i = 0; i < 8; ++i) {
            vec2 off = offsets[i] * offset;
            sum += texture2D(texUnit, uv + off) * weights[i];
        }

        sum /= weightSum;
    }

    if (noise) {
        sum += vec4(texture2D(noiseTexture, gl_FragCoord.xy / noiseTextureSize).rrr, 0.0);
    }

    gl_FragColor = roundedRectangle(uv * blurSize, sum.rgb);
}