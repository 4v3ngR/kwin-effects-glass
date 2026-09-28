uniform float refractionOffsetStrength;
uniform float refractionBevelIntensity;
uniform int bodyRefraction;
uniform float bodyRefractionReach;
uniform float bodyRefractionStrength;

vec4 processSample(sampler2D tex, vec2 baseUv, vec3 glassNormal, float ior, float dispersion, float magnitude, vec2 uvScale, vec2 lensShift)
{
    vec3 viewRay = vec3(0.0, 0.0, -1.0);

    vec3 refractG = refract(viewRay, glassNormal, 1.0 / ior);
    vec2 dir = length(refractG.xy) > 0.001 ? normalize(refractG.xy) : vec2(0.0);
    vec2 shiftG = dir * magnitude * uvScale + lensShift;
    vec4 sampleG = texture(tex, clamp(baseUv + shiftG, 0.0, 1.0));

    if (dispersion > 0.001) {
        float fringe = clamp(dispersion, 0.0, 1.0) * 0.3;
        vec2 shiftR = dir * (magnitude * (1.0 + fringe)) * uvScale + lensShift;
        vec2 shiftB = dir * (magnitude * (1.0 - fringe)) * uvScale + lensShift;

        float r = texture(tex, clamp(baseUv + shiftR, 0.0, 1.0)).r;
        float b = texture(tex, clamp(baseUv + shiftB, 0.0, 1.0)).b;
        return vec4(r, sampleG.g, b, sampleG.a);
    }
    return sampleG;
}

GlassFragment snellsRefraction(vec2 position, vec2 halfBlurSize, vec4 cornerRadius, float minHalfSize, float dist, float concaveFactor)
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
    
    float normalHeight = concaveFactor * refractionBevelIntensity;
    vec2 normalXY = gradLen > 0.001 ? (smoothGrad / gradLen) * normalHeight : vec2(0.0);
    vec3 glassNormal = normalize(vec3(normalXY, 1.0));

    float lensMagnitude = concaveFactor * bandWidth * refractionBevelIntensity;
    vec2 surfaceNormal = gradLen > 0.001 ? smoothGrad / gradLen : vec2(1.0, 0.0);
    vec2 outwardDir = length(surfaceNormal) > 0.001 ? normalize(surfaceNormal) : vec2(0.0);

    vec2 normalizedPos = position / blurSize;
    float cornerWeight = dot(normalizedPos, normalizedPos) * refractionOffsetStrength;
    surfaceNormal += normalizedPos * concaveFactor * cornerWeight;

    vec2 uvScale = 1.0 / blurSize;
    vec2 lensShift = -surfaceNormal * lensMagnitude * uvScale;

    float effectiveBodyReach = min(bodyRefractionReach, minHalfSize);
    if (bodyRefraction == 1 && bodyRefractionStrength > 0.0 && effectiveBodyReach > 0.0 && length(outwardDir) > 0.001) {
        float interiorDist = max(-dist, 0.0);
        float bodyT = 1.0 - clamp(interiorDist / effectiveBodyReach, 0.0, 1.0);
        float bodyProfile = 1.0 - sqrt(max(1.0 - bodyT * bodyT, 0.0));

        vec2 sampleDir = -outwardDir;
        vec2 dirAbs = max(abs(sampleDir), vec2(1e-3));
        vec2 roomToEdge = (halfBlurSize - sign(sampleDir) * position) / dirAbs;
        float available = max(min(roomToEdge.x, roomToEdge.y), 0.0);
        float bodyMagnitude = min(bodyRefractionStrength * refractionStrength * 40.0, available * 0.85);
        lensShift -= outwardDir * bodyMagnitude * bodyProfile * uvScale;
    }

    float refractionMagnitude = lensMagnitude * refractionStrength;
    vec4 color = processSample(texUnit, uv, glassNormal, ior, refractionRGBFringing, refractionMagnitude, uvScale, lensShift);

    return GlassFragment(color, dist, concaveFactor);
}
