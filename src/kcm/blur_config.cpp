/*
    SPDX-FileCopyrightText: 2010 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "blur_config.h"

//#include <config-kwin.h>

// KConfigSkeleton
#include "blurconfig.h"

#include <KPluginFactory>
#include "kwineffects_interface.h"

#include <QFileDialog>
#include <QCheckBox>
#include <QPushButton>

namespace KWin
{

K_PLUGIN_CLASS(BlurEffectConfig)

BlurEffectConfig::BlurEffectConfig(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    ui.setupUi(widget());
    BlurConfig::instance("kwinrc");
    addConfig(BlurConfig::self(), widget());

    auto updateRoundedCornerControls = [this]() {
        const bool useDeclaredCornerRadius = ui.kcfg_UseDeclaredCornerRadius->isChecked();
        const bool customCornerRadius = !useDeclaredCornerRadius;
        const bool dynamicCorners = customCornerRadius && ui.kcfg_DynamicCorners->isChecked();

        ui.labelTopCornerRadius->setEnabled(customCornerRadius);
        ui.kcfg_TopCornerRadius->setEnabled(customCornerRadius);
        ui.labelBottomCornerRadius->setEnabled(customCornerRadius);
        ui.kcfg_BottomCornerRadius->setEnabled(customCornerRadius);
        ui.labelMenuCornerRadius->setEnabled(customCornerRadius);
        ui.kcfg_MenuCornerRadius->setEnabled(customCornerRadius);
        ui.labelDockCornerRadius->setEnabled(customCornerRadius);
        ui.kcfg_DockCornerRadius->setEnabled(customCornerRadius);
        ui.kcfg_RoundCornersOfMaximizedWindows->setEnabled(customCornerRadius);
        ui.kcfg_DynamicCorners->setEnabled(customCornerRadius);
        ui.kcfg_DynamicCornersExcludeDocks->setEnabled(dynamicCorners);
        ui.kcfg_DynamicCornersExcludeTooltips->setEnabled(dynamicCorners);
        ui.kcfg_DynamicCornersExcludeMenus->setEnabled(dynamicCorners);
    };
    updateRoundedCornerControls();
    connect(ui.kcfg_UseDeclaredCornerRadius, &QCheckBox::toggled, this, updateRoundedCornerControls);
    connect(ui.kcfg_DynamicCorners, &QCheckBox::toggled, this, updateRoundedCornerControls);

    QFile about(":/effects/glass/kcm/about.html");
    if (about.open(QIODevice::ReadOnly)) {
        const auto html = about.readAll()
            .replace("${title}", dgettext("kwin_effects_glass", "Glass"))
            .replace("${version}", ABOUT_VERSION_STRING)
            .replace("${repo}", "https://github.com/4v3ngR/kwin-effects-glass");
        ui.aboutText->setHtml(html);
    }

    setupContextualHelp();
}

BlurEffectConfig::~BlurEffectConfig()
{
}

void BlurEffectConfig::setContextualHelp(
    KContextualHelpButton *const contextualHelpButton,
    const QString &text,
    QWidget *const heightHintWidget
)
{
    contextualHelpButton->setContextualHelpText(text);
    if (heightHintWidget) {
        const auto ownHeightHint = contextualHelpButton->sizeHint().height();
        const auto otherHeightHint = heightHintWidget->sizeHint().height();
        if (ownHeightHint >= otherHeightHint) {
            contextualHelpButton->setHeightHintWidget(heightHintWidget);
        }
    }
}

void BlurEffectConfig::setupContextualHelp()
{
    setContextualHelp(
        ui.windowClassesContextualHelp,
        i18n("<p>Specify one window class per line.</p><p>Use <code>$blank</code> to match empty window classes.<br/>Use <code>$$</code> for literal dollar sign.</p>"),
        ui.windowClassesBriefDescription
    );
}

void BlurEffectConfig::save()
{
    KCModule::save();

    OrgKdeKwinEffectsInterface interface(QStringLiteral("org.kde.KWin"),
                                         QStringLiteral("/Effects"),
                                         QDBusConnection::sessionBus());

    if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
        interface.reconfigureEffect(QStringLiteral("glass_x11"));
    } else {
        interface.reconfigureEffect(QStringLiteral("glass"));
    }
}

} // namespace KWin

#include "blur_config.moc"

#include "moc_blur_config.cpp"
