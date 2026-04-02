uniform sampler2D texUnit;
uniform vec2 noiseTextureSize;

VARYING_IN vec2 uv;

void main(void)
{
    vec2 uvNoise = vec2(gl_FragCoord.xy / noiseTextureSize);

    FRAG_COLOR = vec4(TEXTURE(texUnit, uvNoise).rrr, 0);
}
