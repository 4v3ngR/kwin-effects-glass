uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;

VARYING_IN vec2 uv;

void main(void)
{
    vec4 sum = TEXTURE(texUnit, uv) * 4.0;
    sum += TEXTURE(texUnit, uv - halfpixel.xy * offset);
    sum += TEXTURE(texUnit, uv + halfpixel.xy * offset);
    sum += TEXTURE(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset);
    sum += TEXTURE(texUnit, uv - vec2(halfpixel.x, -halfpixel.y) * offset);

    FRAG_COLOR = sum / 8.0;
}
