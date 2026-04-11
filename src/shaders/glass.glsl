uniform vec3 tintColor;
uniform float tintStrength;
uniform vec3 glowColor;
uniform float glowStrength;
uniform int edgeLighting;

uniform float edgeSizePixels;
uniform float refractionStrength;
uniform float refractionNormalPow;
uniform float refractionRGBFringing;

uniform float saturationBoost;
uniform float glassBrightness;
uniform int blendGlowColor;
uniform int boostEdgeSaturation;

vec3 srgbToLinear(vec3 c)
{
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

vec3 linearToSrgb(vec3 c)
{
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

vec3 linearToOklab(vec3 c)
{
    float l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;
    float m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;
    float s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;

    float l_ = pow(max(l, 0.0), 1.0 / 3.0);
    float m_ = pow(max(m, 0.0), 1.0 / 3.0);
    float s_ = pow(max(s, 0.0), 1.0 / 3.0);

    return vec3(
        0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
        1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
        0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_
    );
}

vec3 oklabToLinear(vec3 lab)
{
    float l_ = lab.x + 0.3963377774 * lab.y + 0.2158037573 * lab.z;
    float m_ = lab.x - 0.1055613458 * lab.y - 0.0638541728 * lab.z;
    float s_ = lab.x - 0.0894841775 * lab.y - 1.2914855480 * lab.z;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return vec3(
         4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
        -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
        -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    );
}

vec3 oklabSatBoost(vec3 color, float amount)
{
    if (abs(amount - 1.0) < 0.001) return color;

    vec3 linear = srgbToLinear(clamp(color, 0.0, 1.0));
    vec3 lab = linearToOklab(linear);
    lab.yz *= amount;
    vec3 result = oklabToLinear(lab);
    return linearToSrgb(clamp(result, 0.0, 1.0));
}

float roundedRectangleDist(vec2 pos, vec2 halfSize, vec4 radius)
{
    float r = pos.x > 0.0
        ? (pos.y > 0.0 ? radius.y : radius.w)
        : (pos.y > 0.0 ? radius.x : radius.z);
    vec2 q = abs(pos) - halfSize + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

vec4 glass(vec4 color, vec4 radius)
{
    vec2 halfSize = blurSize * 0.5;
    vec2 pixelPos = uv * blurSize - halfSize;
    float dist = roundedRectangleDist(pixelPos, halfSize, radius);

    if (dist >= 0.0) {
        return color;
    }
    float rimWidth = max(edgeSizePixels * 0.025, 0.9);
    float rimIntensity = exp(-(-dist) / rimWidth);

    float brightness = glassBrightness;
    float ior = 1.0 + refractionStrength;

    if (ior > 1.001) {
        float bandWidth = edgeSizePixels * 0.5;
        float sdfBlend = clamp(-dist / max(bandWidth, 0.1), 0.0, 1.0);
        float sdfProfile = 6.0 * sdfBlend * (1.0 - sdfBlend);

        float eps = bandWidth * 0.75;
        float dxp = roundedRectangleDist(pixelPos + vec2(eps, 0.0), halfSize, radius);
        float dxn = roundedRectangleDist(pixelPos - vec2(eps, 0.0), halfSize, radius);
        float dyp = roundedRectangleDist(pixelPos + vec2(0.0, eps), halfSize, radius);
        float dyn = roundedRectangleDist(pixelPos - vec2(0.0, eps), halfSize, radius);
        vec2 smoothGrad = vec2(dxp - dxn, dyp - dyn);
        float gradLen = length(smoothGrad);

        float normalHeight = min(sdfProfile * refractionNormalPow * 0.15, 2.0);
        vec2 normalXY = gradLen > 0.001
            ? (smoothGrad / gradLen) * normalHeight
            : vec2(0.0);
        vec3 glassNormal = normalize(vec3(normalXY, 1.0));

        vec3 viewRay = vec3(0.0, 0.0, -1.0);
        float refractionDepth = bandWidth;

        float dispersion = refractionRGBFringing * 0.04;
        float iorRed = ior - dispersion;
        float iorBlue = ior + dispersion;

        vec3 refractRed = refract(viewRay, glassNormal, 1.0 / max(iorRed, 1.001));
        vec3 refractGreen = refract(viewRay, glassNormal, 1.0 / max(ior, 1.001));
        vec3 refractBlue = refract(viewRay, glassNormal, 1.0 / max(iorBlue, 1.001));

        if (dot(refractRed, refractRed) < 0.000001) refractRed = vec3(0.0, 0.0, -1.0);
        if (dot(refractGreen, refractGreen) < 0.000001) refractGreen = vec3(0.0, 0.0, -1.0);
        if (dot(refractBlue, refractBlue) < 0.000001) refractBlue = vec3(0.0, 0.0, -1.0);

        float inverseBandWidth = 1.0 / max(bandWidth, 0.1);
        float lensBlend = 1.0 - smoothstep(0.0, 1.0, -dist * inverseBandWidth);
        float lensMagnitude = lensBlend * bandWidth;

        vec2 surfaceNormal = gradLen > 0.001 ? smoothGrad / gradLen : vec2(1.0, 0.0);
        vec2 normalizedPos = pixelPos / blurSize;
        float cornerWeight = dot(normalizedPos, normalizedPos) * 3.0;
        surfaceNormal += normalizedPos * lensBlend * cornerWeight;

        vec2 uvScale = 1.0 / blurSize;
        vec2 lensShift = -surfaceNormal * lensMagnitude * uvScale;

        vec2 uvShiftRed = -refractRed.xy / abs(refractRed.z) * refractionDepth * uvScale + lensShift;
        vec2 uvShiftGreen = -refractGreen.xy / abs(refractGreen.z) * refractionDepth * uvScale + lensShift;
        vec2 uvShiftBlue = -refractBlue.xy / abs(refractBlue.z) * refractionDepth * uvScale + lensShift;

        vec4 sampleGreen = TEXTURE(texUnit, clamp(uv + uvShiftGreen, 0.0, 1.0));
        color.r = TEXTURE(texUnit, clamp(uv + uvShiftRed, 0.0, 1.0)).r;
        color.g = sampleGreen.g;
        color.b = TEXTURE(texUnit, clamp(uv + uvShiftBlue, 0.0, 1.0)).b;
        color.a = sampleGreen.a;

        if (boostEdgeSaturation == 1) {
            float edgeSaturation = 1.0 + lensBlend * 0.5;
            color.rgb = oklabSatBoost(color.rgb, edgeSaturation);
        }

        if (edgeLighting == 1) {
            float edgeBrightness = 1.0 - smoothstep(0.0, bandWidth, -dist);
            brightness += edgeBrightness * glowStrength;
        }
    } else {
        if (boostEdgeSaturation == 1) {
            color.rgb = oklabSatBoost(color.rgb, 1.0 + rimIntensity * 1.5);
        }
    }

    if (blendGlowColor == 1) {
        brightness += rimIntensity * 2.0 * glowStrength;
    }

    if (abs(saturationBoost - 1.0) > 0.001) {
        color.rgb = oklabSatBoost(color.rgb, saturationBoost);
    }

    vec3 tinted = mix(color.rgb, tintColor, clamp(tintStrength, 0.0, 1.0));
    tinted *= min(brightness, 2.5);
    if (blendGlowColor == 1) {
        tinted += glowColor * rimIntensity * glowStrength;
    } else {
        tinted = mix(tinted, glowColor, rimIntensity * glowStrength);
    }

    return vec4(tinted, 1.0);
}
