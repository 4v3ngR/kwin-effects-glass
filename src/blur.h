/*
    SPDX-FileCopyrightText: 2010 Fredrik Höglund <fredrik@kde.org>
    SPDX-FileCopyrightText: 2018 Alex Nemeth <alex.nemeth329@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "effect/effect.h"
#ifndef GLASS_X11
#include "core/region.h"
#endif
#include "opengl/glutils.h"
#include "scene/item.h"
#include "settings.h"

#include <QList>
#include <QStringList>

#include <unordered_map>
#include <Plasma/plasma_version.h>

namespace KWin
{

#if !defined(GLASS_X11) && !defined(GLASS_KWIN_67)
class BlurManagerInterface;
class ContrastManagerInterface;
#endif
class BackgroundEffectItem;

#ifdef GLASS_X11
using BlurOutput = Output;
using BlurRegion = QRegion;
#else
using BlurOutput = RenderView;
using BlurRegion = Region;
#endif

struct BlurRenderData
{
    /// Temporary render targets needed for the Dual Kawase algorithm, the first texture
    /// contains not blurred background behind the window, it's cached.
    std::vector<std::unique_ptr<GLTexture>> textures;
    std::vector<std::unique_ptr<GLFramebuffer>> framebuffers;
};

struct BlurEffectData
{
    /// The region that should be blurred behind the window
    std::optional<BlurRegion> content;

    /// The region that should be blurred behind the frame
    std::optional<BlurRegion> frame;

    /**
     * The render data per render view, as they can have different
     *  color spaces and even different windows on them
     */
    std::unordered_map<BlurOutput *, BlurRenderData> render;
#if PLASMA_VERSION >= 0x060404 && !defined(GLASS_X11)
    std::unique_ptr<BackgroundEffectItem> blurItem;
#endif

    ItemEffect windowEffect;

    /**
     * Color transformation matrix (contrast, and saturation).
     */
    std::optional<QMatrix4x4> colorMatrix;

    /**
     * Corner radius reported by the window before this effect overrides it.
     */
    std::optional<BorderRadius> originalCornerRadius;
};

class BlurEffect : public KWin::Effect
{
    Q_OBJECT

public:
    BlurEffect();
    ~BlurEffect() override;

    static bool supported();
    static bool enabledByDefault();

    void reconfigure(ReconfigureFlags flags) override;
#ifdef GLASS_KWIN_67
    void prePaintScreen(ScreenPrePaintData &data) override;

#ifdef GLASS_X11
    void prePaintWindow(EffectWindow *w, WindowPrePaintData &data) override;
#else
    void prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data) override;
#endif
#else
    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;

#ifdef GLASS_X11
    void prePaintWindow(EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;
#else
    void prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;
#endif
#endif
    void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const BlurRegion &deviceRegion, WindowPaintData &data) override;

    bool provides(Feature feature) override;
    bool isActive() const override;

    int requestedEffectChainPosition() const override
    {
        return 20;
    }

    bool eventFilter(QObject *watched, QEvent *event) override;

    bool blocksDirectScanout() const override;
    bool shouldFlattenCorner(KWin::EffectWindow *w, Qt::Corner corner) const;

public Q_SLOTS:
    void slotWindowAdded(KWin::EffectWindow *w);
    void slotWindowDeleted(KWin::EffectWindow *w);
    void slotOutputRemoved(KWin::BlurOutput *output);
#if KWIN_BUILD_X11
    void slotPropertyNotify(KWin::EffectWindow *w, long atom);
#endif
    void setupDecorationConnections(EffectWindow *w);

private:
    struct BlurPipelineSettings
    {
        size_t iterationCount;
        float offset;
        int expandSize;
        int noiseStrength;
    };

    void initBlurStrengthValues();
    BlurRegion contentRegion(EffectWindow *w, const BorderRadius *fallbackCornerRadius = nullptr) const;
    BlurRegion blurRegion(EffectWindow *w, const BorderRadius *fallbackCornerRadius = nullptr) const;
    BlurRegion roundedContentRegion(const QRect &rect, const BorderRadius &cornerRadius, qreal leftSideWidth, qreal rightSideWidth, qreal topHeight, qreal bottomHeight) const;
    BorderRadius effectiveWindowCornerRadius(EffectWindow *w, const BorderRadius &declaredCornerRadius, bool *isOverRounded = nullptr, bool applyDynamicCorners = true) const;
    QRectF dynamicCornerRect(EffectWindow *w) const;
    BlurRegion decorationBlurRegion(const EffectWindow *w) const;
    bool decorationSupportsBlurBehind(const EffectWindow *w) const;
    bool shouldBlur(const EffectWindow *w, int mask, const WindowPaintData &data) const;
    void updateBlurRegion(EffectWindow *w);
    void repaintDynamicCorners();
    void blur(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const BlurRegion &deviceRegion, WindowPaintData &data);
    GLTexture *ensureNoiseTexture(int noiseStrength);
    QMatrix4x4 colorMatrix(const float &brightness, const float &saturation, const float &contrast) const;
    BlurPipelineSettings pipelineSettingsForStrength(int blurStrength, int noiseStrength) const;

private:
    struct
    {
        std::unique_ptr<GLShader> shader;
        int mvpMatrixLocation;
        int colorMatrixLocation;
        int useOklabSaturationLocation;
        int saturationLocation;
        int offsetLocation;
        int halfpixelLocation;
        int boxLocation;
        int cornerRadiusLocation;
        int opacityLocation;

        int blurSizeLocation;
        int edgeSizePixelsLocation;
        int refractionStrengthLocation;
        int refractionNormalPowLocation;
        int refractionRGBFringingLocation;
        int refractionOffsetStrengthLocation;
        int refractionBevelIntensityLocation;
        int physicallyBasedRefractionLocation;

        int tintColorLocation;
        int tintStrengthLocation;

        int glowColorLocation;
        int glowStrengthLocation;
        int edgeLightingLocation;
    } m_roundedOnscreenPass;

    struct
    {
        std::unique_ptr<GLShader> shader;
        int mvpMatrixLocation;
        int offsetLocation;
        int halfpixelLocation;
    } m_downsamplePass;

    struct
    {
        std::unique_ptr<GLShader> shader;
        int mvpMatrixLocation;
        int offsetLocation;
        int halfpixelLocation;
        int saturationCompensationLocation;
    } m_upsamplePass;

    struct
    {
        std::unique_ptr<GLShader> shader;
        int mvpMatrixLocation;
        int noiseTextureSizeLocation;

        std::unique_ptr<GLTexture> noiseTexture;
        qreal noiseTextureScale = 1.0;
        int noiseTextureStength = 0;
    } m_noisePass;

    BlurSettings m_settings;
    bool m_valid = false;
#if KWIN_BUILD_X11
    long net_wm_blur_region = 0;
#endif
    BlurRegion m_paintedDeviceArea; // keeps track of all painted areas (from bottom to top)
    BlurRegion m_currentDeviceBlur; // keeps track of currently blurred area of the windows (from bottom to top)
    BlurOutput *m_currentOutput = nullptr;

    QMatrix4x4 m_colorMatrix;
    int m_expandSize;
    float m_blurRadius = 1.0f;
    float m_upsampleOffset = 1.0f;
    size_t m_maxIterationCount = 1; // number of times the texture will be downsized to half size
    BlurPipelineSettings m_contentBlurSettings{};
    BlurPipelineSettings m_decorationBlurSettings{};
    BlurPipelineSettings m_dockBlurSettings{};
    QStringList m_windowClasses;
    bool m_whitelist;

    struct OffsetStruct
    {
        float minOffset;
        float maxOffset;
        int expandSize;
    };

    QList<OffsetStruct> blurOffsets;

    struct BlurValuesStruct
    {
        int iteration;
        float offset;
    };

    QList<BlurValuesStruct> blurStrengthValues;

    QMap<EffectWindow *, QMetaObject::Connection> windowBlurChangedConnections;
#if !defined(GLASS_X11) && !defined(GLASS_KWIN_67)
    QMap<EffectWindow *, QMetaObject::Connection> windowContrastChangedConnections;
#endif
    QMap<EffectWindow *, QMetaObject::Connection> windowFrameGeometryChangedConnections;
    std::unordered_map<EffectWindow *, BlurEffectData> m_windows;

#if !defined(GLASS_X11) && !defined(GLASS_KWIN_67)
    static BlurManagerInterface *s_blurManager;
    static QTimer *s_blurManagerRemoveTimer;

    static ContrastManagerInterface *s_contrastManager;
    static QTimer *s_contrastManagerRemoveTimer;
#endif
};

inline bool BlurEffect::provides(Effect::Feature feature)
{
    if (feature == Blur) {
        return true;
    }
    return KWin::Effect::provides(feature);
}

} // namespace KWin
