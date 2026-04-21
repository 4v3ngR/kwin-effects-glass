uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;
uniform float satCompensation;

VARYING_IN vec2 uv;

void main(void)
{
    vec4 sum = TEXTURE(texUnit, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += TEXTURE(texUnit, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += TEXTURE(texUnit, uv + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += TEXTURE(texUnit, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += TEXTURE(texUnit, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += TEXTURE(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += TEXTURE(texUnit, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += TEXTURE(texUnit, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;

    sum /= 12.0;

    if (satCompensation > 1.001) {
        float luma = dot(sum.rgb, vec3(0.2126, 0.7152, 0.0722));
        sum.rgb = mix(vec3(luma), sum.rgb, satCompensation);
    }

    FRAG_COLOR = sum;
}
