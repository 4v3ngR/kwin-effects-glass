#include "settings.h"
#include "blurconfig.h"

#include <algorithm>

namespace KWin
{

QStringList parseWindowClasses(const QString &input)
{
    QStringList result;
    const auto blank = QStringLiteral("blank");
    for (const auto &line : input.split("\n", Qt::SkipEmptyParts)) {
        QString unescaped = "";
        bool consumed = false;
        for (qsizetype i = 0; i < line.size(); i++) {
            const auto character = line[i];
            if (character == QChar('$') && !consumed) {
                consumed = true;
                continue;
            }
            if (consumed) {
                const qsizetype skips = blank.size();
                if (line.mid(i, skips) == blank) {
                    consumed = false;
                    i += skips - 1;
                    continue;
                }
            }
            consumed = false;
            unescaped += character;
        }
        if (consumed) {
            unescaped += QChar('$');
        }
        result << unescaped;
    }
    return result;
}

void BlurSettings::read()
{
    BlurConfig::self()->read();

    general.blurStrength = BlurConfig::blurStrength() - 1;
    general.noiseStrength = BlurConfig::noiseStrength();
    general.decorationBlurStrength = BlurConfig::decorationBlurStrength() - 1;
    general.decorationNoiseStrength = BlurConfig::decorationNoiseStrength();
    general.dockBlurStrength = BlurConfig::dockBlurStrength() - 1;
    general.dockNoiseStrength = BlurConfig::dockNoiseStrength();
    general.brightness = BlurConfig::brightness();
    general.saturation = BlurConfig::saturation();
    general.contrast = BlurConfig::contrast();
    general.oklabSaturation = BlurConfig::oklabSaturation();

    const auto finetuneScale = [](int finetune) {
        return 0.5f + std::clamp(finetune, 0, 10) * 0.13f;
    };
    general.blurRadius = finetuneScale(BlurConfig::blurFinetune());
    general.upsampleOffset = general.blurRadius;
    general.dockBlurRadius = finetuneScale(BlurConfig::dockBlurFinetune());
    general.dockUpsampleOffset = general.dockBlurRadius;
    general.saturationCompensation = BlurConfig::blurSaturationCompensation();

    general.tintColor = BlurConfig::tintColor();
    general.autoTintAlpha = BlurConfig::autoTintAlpha();
    const float autoTintAlphaMin = std::clamp(BlurConfig::autoTintAlphaMin(), 0, 100) / 100.0f;
    const float autoTintAlphaMax = std::clamp(BlurConfig::autoTintAlphaMax(), 0, 100) / 100.0f;
    general.autoTintAlphaMin = std::min(autoTintAlphaMin, autoTintAlphaMax);
    general.autoTintAlphaMax = std::max(autoTintAlphaMin, autoTintAlphaMax);
    general.glowColor = BlurConfig::glowColor();
    general.rimGlow = BlurConfig::rimGlow();
    general.glowOffset = std::clamp(BlurConfig::glowOffset(), -100, 100) / 100.0f;
    general.rimGlowColorMix = BlurConfig::rimGlowColorMix();
    general.rimSpecular = BlurConfig::rimSpecular();
    general.rimSpecularScale = std::clamp(BlurConfig::rimSpecularScale(), 0, 20) / 10.0f;
    general.rimWidth = std::clamp(BlurConfig::rimWidth(), 10, 40) / 10.0f;
    general.rimAdaptToRefraction = BlurConfig::rimAdaptToRefraction();
    general.rimAdaptMultiplier = std::clamp(BlurConfig::rimAdaptMultiplier(), 0, 20) / 10.0f;
    general.edgeLighting = BlurConfig::edgeLighting();
    general.edgeLightingDock = BlurConfig::edgeLightingDock();
    general.edgeLightingTooltip = BlurConfig::edgeLightingTooltip();
    general.excludeDocks = BlurConfig::excludeDocks();
    general.excludeDecorations = BlurConfig::excludeDecorations();
    general.excludeTooltips = BlurConfig::excludeTooltips();
    general.excludeMenus = BlurConfig::excludeMenus();
    general.excludeOSD = BlurConfig::excludeOSD();

    forceBlur.windowClasses = parseWindowClasses(BlurConfig::windowClasses());
    forceBlur.windowClassMatchingMode = BlurConfig::blurMatching() ? WindowClassMatchingMode::Whitelist : WindowClassMatchingMode::Blacklist;
    forceBlur.blurDecorations = BlurConfig::blurDecorations();
    forceBlur.blurMenus = BlurConfig::blurMenus();
    forceBlur.blurDocks = BlurConfig::blurDocks();

    roundedCorners.windowTopRadius = BlurConfig::topCornerRadius();
    roundedCorners.windowBottomRadius = BlurConfig::bottomCornerRadius();
    roundedCorners.menuRadius = BlurConfig::menuCornerRadius();
    roundedCorners.dockRadius = BlurConfig::dockCornerRadius();
    roundedCorners.useDeclaredCornerRadius = BlurConfig::useDeclaredCornerRadius();
    roundedCorners.ignoreContentBlurRegion = BlurConfig::ignoreContentBlurRegion();
    roundedCorners.roundMaximized = BlurConfig::roundCornersOfMaximizedWindows();
    roundedCorners.dynamicCorners = BlurConfig::dynamicCorners();
    roundedCorners.dynamicCornersExcludeWindows = BlurConfig::dynamicCornersExcludeWindows();
    roundedCorners.dynamicCornersExcludeDocks = BlurConfig::dynamicCornersExcludeDocks();
    roundedCorners.dynamicCornersExcludeTooltips = BlurConfig::dynamicCornersExcludeTooltips();
    roundedCorners.dynamicCornersExcludeMenus = BlurConfig::dynamicCornersExcludeMenus();

    refraction.edgeSizePixels = BlurConfig::refractionEdgeSize() * 10;
    refraction.refractionStrength = BlurConfig::refractionStrength() / 20.0;
    refraction.refractionNormalPow = BlurConfig::refractionNormalPow() / 2.0;
    refraction.refractionRGBFringing = BlurConfig::refractionRGBFringing() / 20.0;
    refraction.refractionOffsetStrength = BlurConfig::refractionOffsetStrength() / 2.0;
    refraction.refractionBevelIntensity = BlurConfig::refractionBevelIntensity() / 10.0;
    refraction.physicallyBased = BlurConfig::physicallyBasedRefraction();
    refraction.bodyRefraction = BlurConfig::bodyRefraction();
    refraction.bodyRefractionReach = std::clamp(BlurConfig::bodyRefractionReach(), 0, 80);
    refraction.bodyRefractionStrength = std::clamp(BlurConfig::bodyRefractionStrength(), 0, 100) / 100.0f;
    refraction.excludeWindows = BlurConfig::refractionExcludeWindows();
    refraction.excludeDocks = BlurConfig::refractionExcludeDocks();
    refraction.excludeMenus = BlurConfig::refractionExcludeMenus();
    refraction.excludeTooltips = BlurConfig::refractionExcludeTooltips();
    refraction.excludeOSD = BlurConfig::refractionExcludeOSD();
}

}
