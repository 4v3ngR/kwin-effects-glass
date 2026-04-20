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
    general.brightness = BlurConfig::brightness();
    general.saturation = BlurConfig::saturation();
    general.contrast = BlurConfig::contrast();
    general.blurRadius = std::max(0.2f, BlurConfig::blurRadius() / 10.0f);
    general.upsampleOffset = std::max(0.2f, BlurConfig::upsampleOffset() / 10.0f);
    general.tintColor = BlurConfig::tintColor();
    general.glowColor = BlurConfig::glowColor();
    general.edgeLighting = BlurConfig::edgeLighting();
    general.edgeLightingDock = BlurConfig::edgeLightingDock();
    general.edgeLightingTooltip = BlurConfig::edgeLightingTooltip();
    general.blendGlowColor = BlurConfig::blendGlowColor();
    general.boostEdgeSaturation = BlurConfig::boostEdgeSaturation();
    general.excludeDocks = BlurConfig::excludeDocks();

    forceBlur.windowClasses = parseWindowClasses(BlurConfig::windowClasses());
    forceBlur.windowClassMatchingMode = BlurConfig::blurMatching() ? WindowClassMatchingMode::Whitelist : WindowClassMatchingMode::Blacklist;
    forceBlur.blurDecorations = BlurConfig::blurDecorations();
    forceBlur.blurMenus = BlurConfig::blurMenus();
    forceBlur.blurDocks = BlurConfig::blurDocks();

    roundedCorners.windowTopRadius = BlurConfig::topCornerRadius();
    roundedCorners.windowBottomRadius = BlurConfig::bottomCornerRadius();
    roundedCorners.menuRadius = BlurConfig::menuCornerRadius();
    roundedCorners.dockRadius = BlurConfig::dockCornerRadius();
    roundedCorners.roundMaximized = BlurConfig::roundCornersOfMaximizedWindows();
    roundedCorners.dynamicCorners = BlurConfig::dynamicCorners();

    refraction.edgeSizePixels = BlurConfig::refractionEdgeSize() * 10;
    refraction.refractionStrength = BlurConfig::refractionStrength() / 20.0;
    refraction.refractionNormalPow = BlurConfig::refractionNormalPow() / 2.0;
    refraction.refractionRGBFringing = BlurConfig::refractionRGBFringing() / 20.0;
    refraction.refractionOffsetStrength = BlurConfig::refractionOffsetStrength() / 2.0;

}

}
