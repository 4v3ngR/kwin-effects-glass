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
    QString tintColor;
    QString glowColor;
    bool edgeLighting;
    bool edgeLightingDock;
    bool edgeLightingTooltip;
    bool excludeDocks;
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
    bool roundMaximized;
    bool dynamicCorners;
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
    bool physicallyBased;
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
