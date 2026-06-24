uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;
uniform float saturationCompensation;

in vec2 uv;

void main(void)
{
    vec4 sum = texture(texUnit, uv) * 4.0;

    sum += texture(texUnit, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;

    sum /= 16.0;

    if (saturationCompensation > 1.001) {
        float luma = dot(sum.rgb, vec3(0.2126, 0.7152, 0.0722));
        float lumaWeight = smoothstep(0.04, 0.25, luma);
        float effectiveBoost = mix(1.0, saturationCompensation, lumaWeight);
        sum.rgb = mix(vec3(luma), sum.rgb, effectiveBoost);
    }

    fragColor = sum;
}
