#include "sdf.glsl"

VARYING_IN vec2 uv;
VARYING_IN vec2 vertex;

uniform sampler2D texUnit;
uniform mat4 colorMatrix;
uniform float offset;
uniform vec2 halfpixel;
uniform vec4 box;
uniform vec4 cornerRadius;
uniform float opacity;
uniform vec2 blurSize;

uniform float noiseStrength;
uniform vec3 windowData;

float hashNoise(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hashNoise(i);
    float b = hashNoise(i + vec2(1.0, 0.0));
    float c = hashNoise(i + vec2(0.0, 1.0));
    float d = hashNoise(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float glassNoise(vec2 p)
{
    float n = 0.0;
    n += valueNoise(p * 1.0) * 0.5;
    n += valueNoise(p * 2.0) * 0.25;
    n += valueNoise(p * 4.0) * 0.125;
    n += valueNoise(p * 8.0) * 0.0625;
    return n / 0.95;
}

#include "glass.glsl"

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
        vec2 noiseCoord = (gl_FragCoord.xy - windowData.xy) * 0.8 + windowData.z;
        float n = (glassNoise(noiseCoord) - 0.5) * noiseStrength * 2.0;
        float detail = (hashNoise(gl_FragCoord.xy - windowData.xy) - 0.5) * noiseStrength * 0.3;
        result.rgb += vec3(n + detail);
    }

    FRAG_COLOR = result;
}
