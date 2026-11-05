uniform vec3 glowColor;
uniform float glowStrength;
uniform int edgeLighting;
uniform int rimGlow;
uniform float glowOffset;
uniform int rimGlowColorMix;
uniform int rimSpecular;
uniform float rimSpecularScale;
uniform float rimWidth;
uniform int rimSpecularTopLeft;
uniform int rimSpecularTopRight;
uniform int rimSpecularBottomLeft;
uniform int rimSpecularBottomRight;
uniform vec3 rimSpecularTopLeftColor;
uniform vec3 rimSpecularTopRightColor;
uniform vec3 rimSpecularBottomLeftColor;
uniform vec3 rimSpecularBottomRightColor;
uniform int rimAdaptToRefraction;
uniform float rimAdaptMultiplier;

vec3 glassGlow(vec2 position, GlassFragment s)
{
    vec3 outline = s.color.rgb;

    if (edgeLighting == 1) {
        outline += (s.color.rgb * s.concaveFactor);
    }

    return outline;
}

vec3 glowTintFromBackdrop(vec3 glow, vec3 backdrop)
{
    vec3 glowLab = linearToOklab(srgbToLinear(clamp(glow, 0.0, 1.0)));
    vec3 backLab = linearToOklab(srgbToLinear(clamp(backdrop, 0.0, 1.0)));

    float glowChroma = length(glowLab.gb);
    float cap = max(0.30, glowChroma);
    vec2 chroma = glowLab.gb + backLab.gb * 2.0;
    float c = length(chroma);
    if (c > cap) {
        chroma *= cap / c;
        c = cap;
    }

    float added = max(c - glowChroma, 0.0);
    float lum = clamp(glowLab.x + added * (backLab.x - glowLab.x - 0.85), 0.0, 1.0);
    return linearToSrgb(clamp(oklabToLinear(vec3(lum, chroma)), 0.0, 1.0));
}

vec3 glassOutline(vec2 position, GlassFragment s, vec3 untinted)
{
    vec3 outline = s.color.rgb;
    vec2 halfSize = blurSize * 0.5;
    float bevelScale = physicallyBasedRefraction == 1 ? refractionBevelIntensity : 1.0;
    float edgeBand = max(clamp(edgeSizePixels, 0.1, min(halfSize.x, halfSize.y) * 0.9) * bevelScale * rimAdaptMultiplier, 0.1);
    float width = rimAdaptToRefraction == 1 ? edgeBand / 3.5 : rimWidth;

    if (rimGlow == 1 && glowStrength > 0.0) {
        vec2 edgeDist = halfSize - abs(position);
        vec2 n = position / halfSize;
        float cornerBlend = min(blurSize.x, blurSize.y) * 0.25;
        float horizontalEdge = smoothstep(-cornerBlend, cornerBlend, edgeDist.x - edgeDist.y);
        vec2 lightPos = vec2(glowOffset, 1.0);
        float pointLight = max(1.0 - smoothstep(0.0, 1.0, distance(n, lightPos)),
                               1.0 - smoothstep(0.0, 1.0, distance(n, -lightPos)));
        float lightWeight = mix(horizontalEdge, pointLight, abs(glowOffset));
        float falloff = exp(s.dist / (2.0 * width));
        if (rimAdaptToRefraction == 1) {
            float edge = 1.0 - clamp(-s.dist / edgeBand, 0.0, 1.0);
            falloff = 1.0 - sqrt(1.0 - pow(smoothstep(0.0, 1.0, edge), refractionNormalPow));
        }
        float glowMask = glowStrength * lightWeight * falloff;
        vec3 tint = rimGlowColorMix == 1 ? glowTintFromBackdrop(glowColor, untinted) : glowColor;
        outline = mix(outline, tint, glowMask);
    }

    if (rimSpecular == 1 && rimSpecularScale > 0.0) {
        vec3 specColor = mix(glowColor, vec3(1.0), 0.8);
        if(glowStrength == 0.0 || dot(glowColor, glowColor) <= 0.0) {
            specColor = vec3(1.0);
        }

        float specWidth = width * rimSpecularScale;
        float edgeMask = smoothstep(0.0, -2.0 * specWidth, s.dist);
        float borderInner = smoothstep(-1.0 * specWidth, -3.0 * specWidth, s.dist);
        float edgeProfile = edgeMask - borderInner;
        float thicknessShadow = pow(edgeProfile, 0.9);

        float leftMask = 1.0 - smoothstep(-blurSize.x * 0.7, blurSize.x * 0.7, position.x);
        float rightMask = smoothstep(-blurSize.x * 0.7, blurSize.x * 0.7, position.x);
        float bottomMask = 1.0 - smoothstep(-blurSize.y * 0.7, blurSize.y * 0.7, position.y);
        float topMask = smoothstep(-blurSize.y * 0.7, blurSize.y * 0.7, position.y);

        float tlMask = topMask * leftMask * float(rimSpecularTopLeft);
        float trMask = topMask * rightMask * float(rimSpecularTopRight);
        float blMask = bottomMask * leftMask * float(rimSpecularBottomLeft);
        float brMask = bottomMask * rightMask * float(rimSpecularBottomRight);
        outline = mix(outline, rimSpecularTopLeftColor, clamp(thicknessShadow * tlMask, 0.0, 1.0));
        outline = mix(outline, rimSpecularTopRightColor, clamp(thicknessShadow * trMask, 0.0, 1.0));
        outline = mix(outline, rimSpecularBottomLeftColor, clamp(thicknessShadow * blMask, 0.0, 1.0));
        outline = mix(outline, rimSpecularBottomRightColor, clamp(thicknessShadow * brMask, 0.0, 1.0));

    }

    return outline;
}
