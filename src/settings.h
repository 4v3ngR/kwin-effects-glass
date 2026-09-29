#pragma once

#include <QStringList>

namespace KWin
{

QStringList parseWindowClasses(const QString &input);

enum class WindowClassMatchingMode
{
    Blacklist,
    Whitelist
};


struct GeneralSettings
{
    int blurStrength;
    int noiseStrength;
    int decorationBlurStrength;
    int decorationNoiseStrength;
    int dockBlurStrength;
    int dockNoiseStrength;
    float brightness;
    float saturation;
    float contrast;
    bool oklabSaturation;
    float blurRadius;
    float upsampleOffset;
    float dockBlurRadius;
    float dockUpsampleOffset;
    bool saturationCompensation;
    QString tintColor;
    bool autoTintAlpha;
    float autoTintAlphaMin;
    float autoTintAlphaMax;
    QString glowColor;
    bool rimGlow;
    float glowOffset;
    bool rimGlowColorMix;
    bool rimSpecular;
    float rimSpecularScale;
    float rimWidth;
    bool rimAdaptToRefraction;
    float rimAdaptMultiplier;
    bool edgeLighting;
    bool edgeLightingDock;
    bool edgeLightingTooltip;
    bool excludeDocks;
    bool excludeDecorations;
    bool excludeTooltips;
    bool excludeMenus;
    bool excludeOSD;
};

struct ForceBlurSettings
{
    QStringList windowClasses;
    WindowClassMatchingMode windowClassMatchingMode;
    bool blurDecorations;
    bool blurMenus;
    bool blurDocks;
};

struct RoundedCornersSettings
{
    float windowTopRadius;
    float windowBottomRadius;
    float menuRadius;
    float dockRadius;
    bool useDeclaredCornerRadius;
    bool ignoreContentBlurRegion;
    bool roundMaximized;
    bool dynamicCorners;
    bool dynamicCornersExcludeWindows;
    bool dynamicCornersExcludeDocks;
    bool dynamicCornersExcludeTooltips;
    bool dynamicCornersExcludeMenus;
};

struct RefractionSettings
{
    float edgeSizePixels;
    float refractionStrength;
    float refractionNormalPow;
    float refractionRGBFringing;
    float refractionOffsetStrength;
    float refractionBevelIntensity;
    bool physicallyBased;
    bool bodyRefraction;
    float bodyRefractionReach;
    float bodyRefractionStrength;
    bool excludeWindows;
    bool excludeDocks;
    bool excludeMenus;
    bool excludeTooltips;
    bool excludeOSD;
};

class BlurSettings
{
public:
    GeneralSettings general{};
    ForceBlurSettings forceBlur{};
    RoundedCornersSettings roundedCorners{};
    RefractionSettings refraction{};

    void read();
};

}
