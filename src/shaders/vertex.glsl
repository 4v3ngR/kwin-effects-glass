uniform mat4 modelViewProjectionMatrix;

ATTRIBUTE vec2 position;
ATTRIBUTE vec2 texcoord;

VARYING_OUT vec2 uv;

void main(void)
{
    gl_Position = modelViewProjectionMatrix * vec4(position, 0.0, 1.0);
    uv = texcoord;
}
