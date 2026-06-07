vec4 processSample(sampler2D tex, vec2 baseUv, vec3 glassNormal, float ior, float dispersion, float bandWidth, vec2 uvScale, vec2 lensShift)
{
    vec3 viewRay = vec3(0.0, 0.0, -1.0);

    vec3 refractG = refract(viewRay, glassNormal, 1.0 / ior);
    vec2 shiftG = (-refractG.xy / max(abs(refractG.z), 0.001)) * bandWidth * uvScale + lensShift;
    vec4 sampleG = TEXTURE(tex, clamp(baseUv + shiftG, 0.0, 1.0));

    if (dispersion > 0.001) {
        vec3 refractR = refract(viewRay, glassNormal, 1.0 / (ior - dispersion));
        vec2 shiftR = (-refractR.xy / max(abs(refractR.z), 0.001)) * bandWidth * uvScale + lensShift;

        vec3 refractB = refract(viewRay, glassNormal, 1.0 / (ior + dispersion));
        vec2 shiftB = (-refractB.xy / max(abs(refractB.z), 0.001)) * bandWidth * uvScale + lensShift;

        float r = TEXTURE(tex, clamp(baseUv + shiftR, 0.0, 1.0)).r;
        float b = TEXTURE(tex, clamp(baseUv + shiftB, 0.0, 1.0)).b;
        return vec4(r, sampleG.g, b, sampleG.a);
    }
    return sampleG;
}

GlassFragment snellsRefraction(vec2 position, vec2 halfBlurSize, vec4 cornerRadius, float minHalfSize, float dist, float edgeFactor, float concaveFactor)
{
    float bandWidth = clamp(edgeSizePixels, 0.1, minHalfSize * 0.9);
    float ior = 1.0 + refractionStrength;

    float minR = min(min(cornerRadius.x, cornerRadius.y), min(cornerRadius.z, cornerRadius.w));
    float eps = min(bandWidth * 0.75, minR * 0.6);
    float dxp = roundedRectangleDist(position + vec2(eps, 0.0), halfBlurSize, cornerRadius);
    float dxn = roundedRectangleDist(position - vec2(eps, 0.0), halfBlurSize, cornerRadius);
    float dyp = roundedRectangleDist(position + vec2(0.0, eps), halfBlurSize, cornerRadius);
    float dyn = roundedRectangleDist(position - vec2(0.0, eps), halfBlurSize, cornerRadius);
    vec2 smoothGrad = vec2(dxp - dxn, dyp - dyn);
    float gradLen = length(smoothGrad);

    float normalHeight = min(concaveFactor * 0.4, 0.25) * refractionBevelIntensity;
    vec2 normalXY = gradLen > 0.001 ? (smoothGrad / gradLen) * normalHeight : vec2(0.0);
    vec3 glassNormal = normalize(vec3(normalXY, 1.0));

    float lensMagnitude = concaveFactor * bandWidth * refractionBevelIntensity;
    vec2 surfaceNormal = gradLen > 0.001 ? smoothGrad / gradLen : vec2(1.0, 0.0);

    vec2 normalizedPos = position / blurSize;
    float cornerWeight = dot(normalizedPos, normalizedPos) * refractionOffsetStrength;
    surfaceNormal += normalizedPos * concaveFactor * cornerWeight;

    vec2 uvScale = 1.0 / blurSize;
    vec2 lensShift = -surfaceNormal * lensMagnitude * uvScale;

    vec4 color = processSample(texUnit, uv, glassNormal, ior, refractionRGBFringing, bandWidth, uvScale, lensShift);

    return GlassFragment(color, dist, edgeFactor, concaveFactor, glassNormal, ior);
}
