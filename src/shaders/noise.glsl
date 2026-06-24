uniform sampler2D texUnit;
uniform vec2 noiseTextureSize;

in vec2 uv;

void main(void)
{
    vec2 uvNoise = vec2(gl_FragCoord.xy / noiseTextureSize);

    fragColor = vec4(texture(texUnit, uvNoise).rrr, 0);
}
