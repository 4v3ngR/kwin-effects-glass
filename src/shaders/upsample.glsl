uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;

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

    FRAG_COLOR = sum / 12.0;
}
