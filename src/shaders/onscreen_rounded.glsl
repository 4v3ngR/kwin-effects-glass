#include "sdf.glsl"

uniform sampler2D texUnit;
uniform mat4 colorMatrix;
uniform float offset;
uniform vec2 halfpixel;
uniform vec4 box;
uniform vec4 cornerRadius;
uniform float opacity;
uniform vec2 blurSize;

VARYING_IN vec2 uv;
VARYING_IN vec2 vertex;

#include "glass.glsl"
#include "noise.glsl"

void main(void)
{
    vec2 halfBlurSize = blurSize * 0.5;
    float minHalfSize = min(halfBlurSize.x, halfBlurSize.y);

    vec2 position = uv * blurSize - halfBlurSize.xy;
    float dist = roundedRectangleDist(position, halfBlurSize, cornerRadius);

    vec4 sum = vec4(0);
    if (dist <= 0.0) {
        sum = TEXTURE(texUnit, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);
        sum += TEXTURE(texUnit, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
        sum += TEXTURE(texUnit, uv + vec2(0.0, halfpixel.y * 2.0) * offset);
        sum += TEXTURE(texUnit, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
        sum += TEXTURE(texUnit, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);
        sum += TEXTURE(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
        sum += TEXTURE(texUnit, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);
        sum += TEXTURE(texUnit, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;
        sum /= 12.0;
    }

    sum = glass(sum, cornerRadius);

    float f = sdfRoundedBox(vertex, box.xy, box.zw, cornerRadius);
    float df = fwidth(f);
    sum *= 1.0 - clamp(0.5 + f / df, 0.0, 1.0);

    vec4 result = sum * colorMatrix * opacity;

    if (noiseStrength > 0.0) {
        float n = (hashNoise(gl_FragCoord.xy - windowPosition) - 0.5) * noiseStrength;
        result.rgb += vec3(n);
    }

    FRAG_COLOR = result;
}
