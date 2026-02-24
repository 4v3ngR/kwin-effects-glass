uniform float topCornerRadius;
uniform float bottomCornerRadius;

uniform vec2 blurSize;
uniform float opacity;

// source: https://iquilezles.org/articles/distfunctions2d/
// https://www.shadertoy.com/view/4llXD7
float roundedRectangleDist(vec2 p, vec2 b, float topRadius, float bottomRadius)
{
    float r = (p.y > 0.0) ? topRadius : bottomRadius;
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

vec4 roundedRectangle(vec2 fragCoord, vec3 texture)
{
    if (topCornerRadius == 0 && bottomCornerRadius == 0) {
        return vec4(texture, opacity);
    }

    vec2 halfblurSize = blurSize * 0.5;
    vec2 p = fragCoord - halfblurSize;
    float dist = roundedRectangleDist(p, halfblurSize, topCornerRadius, bottomCornerRadius) * 10.0;
    if (dist < 0.0) {
        return vec4(texture, 1.0);
    }

    float s = smoothstep(0.0, 10.0,  dist);
    return vec4(texture, mix(1.0, 0.0, s));
}
