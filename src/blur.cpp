/*
    SPDX-FileCopyrightText: 2010 Fredrik Höglund <fredrik@kde.org>
    SPDX-FileCopyrightText: 2011 Philipp Knechtges <philipp-dev@knechtges.com>
    SPDX-FileCopyrightText: 2018 Alex Nemeth <alex.nemeth329@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "blur.h"
// KConfigSkeleton
#include "blurconfig.h"
#include "settings.h"

#include "core/pixelgrid.h"
#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glplatform.h"
#include "scene/decorationitem.h"
#include "scene/scene.h"
#include "scene/surfaceitem.h"
#include "scene/windowitem.h"
#include "wayland/blur.h"
#include "wayland/contrast.h"
#include "wayland/display.h"
#include "wayland/surface.h"
#include "window.h"

#if KWIN_BUILD_X11
#include "utils/xcbutils.h"
#endif

#include <QGuiApplication>
#include <QMatrix4x4>
#include <QScreen>
#include <QTime>
#include <QTimer>
#include <QWindow>
#include <cmath> // for ceil()
#include <cstdlib>

#include <KConfigGroup>
#include <KSharedConfig>

#include <KDecoration3/Decoration>

Q_LOGGING_CATEGORY(KWIN_BLUR, "kwin_effect_blur", QtWarningMsg)

static void ensureResources()
{
    // Must initialize resources manually because the effect is a static lib.
    Q_INIT_RESOURCE(blur);
}

namespace KWin
{

static const QByteArray s_blurAtomName = QByteArrayLiteral("_KDE_NET_WM_BLUR_BEHIND_REGION");

BlurManagerInterface *BlurEffect::s_blurManager = nullptr;
QTimer *BlurEffect::s_blurManagerRemoveTimer = nullptr;

ContrastManagerInterface *BlurEffect::s_contrastManager = nullptr;
QTimer *BlurEffect::s_contrastManagerRemoveTimer = nullptr;

static QMatrix4x4 colorTransformMatrix(qreal saturation, qreal contrast, qreal brightness)
{
    QMatrix4x4 saturationMatrix;
    QMatrix4x4 contrastMatrix;
    QMatrix4x4 brightnessMatrix;

    if (!qFuzzyCompare(saturation, 1.0)) {
        const qreal rval = (1.0 - saturation) * 0.2126;
        const qreal gval = (1.0 - saturation) * 0.7152;
        const qreal bval = (1.0 - saturation) * 0.0722;

        saturationMatrix = QMatrix4x4(rval + saturation, rval, rval, 0.0,
                                      gval, gval + saturation, gval, 0.0,
                                      bval, bval, bval + saturation, 0.0,
                                      0.0, 0.0, 0.0, 1.0);
    }

    if (!qFuzzyCompare(contrast, 1.0)) {
        const float transl = (1.0 - contrast) / 2.0;

        contrastMatrix = QMatrix4x4(contrast, 0.0, 0.0, 0.0,
                                    0.0, contrast, 0.0, 0.0,
                                    0.0, 0.0, contrast, 0.0,
                                    transl, transl, transl, 1.0);
    }

    if (!qFuzzyCompare(brightness, 1.0)) {
        brightnessMatrix.scale(brightness, brightness, brightness);
    }

    return contrastMatrix * saturationMatrix * brightnessMatrix;
}

BlurEffect::BlurEffect()
{
    BlurConfig::instance(effects->config());
    ensureResources();

    m_roundedOnscreenPass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                                     QStringLiteral(":/effects/glass/generated/onscreen_rounded.vert"),
                                                                                     QStringLiteral(":/effects/glass/generated/onscreen_rounded.frag"));
    if (!m_roundedOnscreenPass.shader) {
        qCWarning(KWIN_BLUR) << "Failed to load onscreen pass shader";
        return;
    } else {
        m_roundedOnscreenPass.mvpMatrixLocation = m_roundedOnscreenPass.shader->uniformLocation("modelViewProjectionMatrix");
        m_roundedOnscreenPass.colorMatrixLocation = m_roundedOnscreenPass.shader->uniformLocation("colorMatrix");
        m_roundedOnscreenPass.offsetLocation = m_roundedOnscreenPass.shader->uniformLocation("offset");
        m_roundedOnscreenPass.halfpixelLocation = m_roundedOnscreenPass.shader->uniformLocation("halfpixel");
        m_roundedOnscreenPass.boxLocation = m_roundedOnscreenPass.shader->uniformLocation("box");
        m_roundedOnscreenPass.cornerRadiusLocation = m_roundedOnscreenPass.shader->uniformLocation("cornerRadius");
        m_roundedOnscreenPass.opacityLocation = m_roundedOnscreenPass.shader->uniformLocation("opacity");
        m_roundedOnscreenPass.blurSizeLocation = m_roundedOnscreenPass.shader->uniformLocation("blurSize");
        m_roundedOnscreenPass.edgeSizePixelsLocation = m_roundedOnscreenPass.shader->uniformLocation("edgeSizePixels");
        m_roundedOnscreenPass.refractionStrengthLocation = m_roundedOnscreenPass.shader->uniformLocation("refractionStrength");
        m_roundedOnscreenPass.refractionNormalPowLocation = m_roundedOnscreenPass.shader->uniformLocation("refractionNormalPow");
        m_roundedOnscreenPass.refractionRGBFringingLocation = m_roundedOnscreenPass.shader->uniformLocation("refractionRGBFringing");
        m_roundedOnscreenPass.tintColorLocation = m_roundedOnscreenPass.shader->uniformLocation("tintColor");
        m_roundedOnscreenPass.tintStrengthLocation = m_roundedOnscreenPass.shader->uniformLocation("tintStrength");
        m_roundedOnscreenPass.glowColorLocation = m_roundedOnscreenPass.shader->uniformLocation("glowColor");
        m_roundedOnscreenPass.glowStrengthLocation = m_roundedOnscreenPass.shader->uniformLocation("glowStrength");
        m_roundedOnscreenPass.edgeLightingLocation = m_roundedOnscreenPass.shader->uniformLocation("edgeLighting");
    }

    m_downsamplePass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                                QStringLiteral(":/effects/glass/generated/vertex.vert"),
                                                                                QStringLiteral(":/effects/glass/generated/downsample.frag"));
    if (!m_downsamplePass.shader) {
        qCWarning(KWIN_BLUR) << "Failed to load downsampling pass shader";
        return;
    } else {
        m_downsamplePass.mvpMatrixLocation = m_downsamplePass.shader->uniformLocation("modelViewProjectionMatrix");
        m_downsamplePass.offsetLocation = m_downsamplePass.shader->uniformLocation("offset");
        m_downsamplePass.halfpixelLocation = m_downsamplePass.shader->uniformLocation("halfpixel");
    }

    m_upsamplePass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                              QStringLiteral(":/effects/glass/generated/vertex.vert"),
                                                                              QStringLiteral(":/effects/glass/generated/upsample.frag"));
    if (!m_upsamplePass.shader) {
        qCWarning(KWIN_BLUR) << "Failed to load upsampling pass shader";
        return;
    } else {
        m_upsamplePass.mvpMatrixLocation = m_upsamplePass.shader->uniformLocation("modelViewProjectionMatrix");
        m_upsamplePass.offsetLocation = m_upsamplePass.shader->uniformLocation("offset");
        m_upsamplePass.halfpixelLocation = m_upsamplePass.shader->uniformLocation("halfpixel");
    }

    m_noisePass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                           QStringLiteral(":/effects/glass/generated/vertex.vert"),
                                                                           QStringLiteral(":/effects/glass/generated/noise.frag"));
    if (!m_noisePass.shader) {
        qCWarning(KWIN_BLUR) << "Failed to load noise pass shader";
        return;
    } else {
        m_noisePass.mvpMatrixLocation = m_noisePass.shader->uniformLocation("modelViewProjectionMatrix");
        m_noisePass.noiseTextureSizeLocation = m_noisePass.shader->uniformLocation("noiseTextureSize");
    }

    initBlurStrengthValues();
    reconfigure(ReconfigureAll);

#if KWIN_BUILD_X11
    if (effects->xcbConnection()) {
        net_wm_blur_region = effects->announceSupportProperty(s_blurAtomName, this);
    }
#endif

    if (!s_blurManagerRemoveTimer) {
        s_blurManagerRemoveTimer = new QTimer(QCoreApplication::instance());
        s_blurManagerRemoveTimer->setSingleShot(true);
        s_blurManagerRemoveTimer->callOnTimeout([]() {
            s_blurManager->remove();
            s_blurManager = nullptr;
        });
    }
    s_blurManagerRemoveTimer->stop();
    if (!s_blurManager) {
        s_blurManager = new BlurManagerInterface(effects->waylandDisplay(), s_blurManagerRemoveTimer);
    }

    if (!s_contrastManagerRemoveTimer) {
        s_contrastManagerRemoveTimer = new QTimer(QCoreApplication::instance());
        s_contrastManagerRemoveTimer->setSingleShot(true);
        s_contrastManagerRemoveTimer->callOnTimeout([]() {
            s_contrastManager->remove();
            s_contrastManager = nullptr;
        });
    }
    s_contrastManagerRemoveTimer->stop();
    if (!s_contrastManager) {
        s_contrastManager = new ContrastManagerInterface(effects->waylandDisplay(), s_contrastManagerRemoveTimer);
    }

    connect(effects, &EffectsHandler::windowAdded, this, &BlurEffect::slotWindowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &BlurEffect::slotWindowDeleted);
    connect(effects, &EffectsHandler::viewRemoved, this, &BlurEffect::slotViewRemoved);
#if KWIN_BUILD_X11
    connect(effects, &EffectsHandler::propertyNotify, this, &BlurEffect::slotPropertyNotify);
    connect(effects, &EffectsHandler::xcbConnectionChanged, this, [this]() {
        net_wm_blur_region = effects->announceSupportProperty(s_blurAtomName, this);
    });
#endif

    // Fetch the blur regions for all windows
    const auto stackingOrder = effects->stackingOrder();
    for (EffectWindow *window : stackingOrder) {
        slotWindowAdded(window);
    }

    m_valid = true;
}

BlurEffect::~BlurEffect()
{
    // When compositing is restarted, avoid removing the manager immediately.
    if (s_blurManager) {
        s_blurManagerRemoveTimer->start(1000);
    }

    if (s_contrastManager) {
        s_contrastManagerRemoveTimer->start(1000);
    }
}

void BlurEffect::initBlurStrengthValues()
{
    // This function creates an array of blur strength values that are evenly distributed

    // The range of the slider on the blur settings UI
    int numOfBlurSteps = 15;
    int remainingSteps = numOfBlurSteps;

    /*
     * Explanation for these numbers:
     *
     * The texture blur amount depends on the downsampling iterations and the offset value.
     * By changing the offset we can alter the blur amount without relying on further downsampling.
     * But there is a minimum and maximum value of offset per downsample iteration before we
     * get artifacts.
     *
     * The minOffset variable is the minimum offset value for an iteration before we
     * get blocky artifacts because of the downsampling.
     *
     * The maxOffset value is the maximum offset value for an iteration before we
     * get diagonal line artifacts because of the nature of the dual kawase blur algorithm.
     *
     * The expandSize value is the minimum value for an iteration before we reach the end
     * of a texture in the shader and sample outside of the area that was copied into the
     * texture from the screen.
     */

    // {minOffset, maxOffset, expandSize}
    blurOffsets.append({1.0, 2.0, 10}); // Down sample size / 2
    blurOffsets.append({2.0, 3.0, 20}); // Down sample size / 4
    blurOffsets.append({2.0, 5.0, 50}); // Down sample size / 8
    blurOffsets.append({3.0, 8.0, 150}); // Down sample size / 16
    // blurOffsets.append({5.0, 10.0, 400}); // Down sample size / 32
    // blurOffsets.append({7.0, ?.0});       // Down sample size / 64

    float offsetSum = 0;

    for (int i = 0; i < blurOffsets.size(); i++) {
        offsetSum += blurOffsets[i].maxOffset - blurOffsets[i].minOffset;
    }

    for (int i = 0; i < blurOffsets.size(); i++) {
        int iterationNumber = std::ceil((blurOffsets[i].maxOffset - blurOffsets[i].minOffset) / offsetSum * numOfBlurSteps);
        remainingSteps -= iterationNumber;

        if (remainingSteps < 0) {
            iterationNumber += remainingSteps;
        }

        float offsetDifference = blurOffsets[i].maxOffset - blurOffsets[i].minOffset;

        for (int j = 1; j <= iterationNumber; j++) {
            // {iteration, offset}
            blurStrengthValues.append({i + 1, blurOffsets[i].minOffset + (offsetDifference / iterationNumber) * j});
        }
    }
}

void BlurEffect::reconfigure(ReconfigureFlags flags)
{
    m_settings.read();

    m_iterationCount = blurStrengthValues[m_settings.general.blurStrength].iteration;
    m_offset = blurStrengthValues[m_settings.general.blurStrength].offset;
    m_expandSize = blurOffsets[m_iterationCount - 1].expandSize;
    m_noiseStrength = m_settings.general.noiseStrength;
    m_colorMatrix = colorTransformMatrix(
        m_settings.general.saturation,
        m_settings.general.contrast,
        m_settings.general.brightness
    );

    m_whitelist = (m_settings.forceBlur.windowClassMatchingMode == WindowClassMatchingMode::Whitelist);
    m_windowClasses = m_settings.forceBlur.windowClasses;

    // Update all windows for the blur to take effect
    effects->addRepaintFull();
}

void BlurEffect::updateBlurRegion(EffectWindow *w)
{
    std::optional<Region> content;
    std::optional<Region> frame;
    std::optional<qreal> saturation;
    std::optional<qreal> contrast;

#if KWIN_BUILD_X11
    if (net_wm_blur_region != XCB_ATOM_NONE) {
        const QByteArray value = w->readProperty(net_wm_blur_region, XCB_ATOM_CARDINAL, 32);
        Region region;
        if (value.size() > 0 && !(value.size() % (4 * sizeof(uint32_t)))) {
            const uint32_t *cardinals = reinterpret_cast<const uint32_t *>(value.constData());
            for (unsigned int i = 0; i < value.size() / sizeof(uint32_t);) {
                int x = cardinals[i++];
                int y = cardinals[i++];
                int w = cardinals[i++];
                int h = cardinals[i++];
                region += Xcb::fromXNative(Rect(x, y, w, h)).toRect();
            }
        }
        if (!value.isNull()) {
            content = region;
        }
    }
#endif

    if (SurfaceInterface *surface = w->surface()) {
        if (surface->blur()) {
            content = surface->blur()->region();
        }
        if (surface->contrast()) {
            saturation = surface->contrast()->saturation();
            contrast = surface->contrast()->contrast();
        }
    }

    if (auto internal = w->internalWindow()) {
        const auto property = internal->property("kwin_blur");
        if (property.isValid()) {
            content = property.value<Region>();
        }
    }

    if (w->decorationHasAlpha() && decorationSupportsBlurBehind(w)) {
        frame = decorationBlurRegion(w);
    }

    if (
        m_settings.forceBlur.blurDecorations &&
        !(
            w->isDock() ||
            w->isMenu() ||
            w->isDropdownMenu() ||
            w->isPopupMenu() ||
            w->isPopupWindow()
        )
    ) {
        frame = Rect(w->frameGeometry().translated(-w->x(), -w->y()).toRect());
    }

    if (content.has_value() || frame.has_value()) {
        BlurEffectData &data = m_windows[w];
        data.content = content;
        data.frame = frame;
        data.colorMatrix = colorTransformMatrix(saturation.value_or(1.0), contrast.value_or(1.0), 1.0);
        data.windowEffect = ItemEffect(w->windowItem());
    } else {
        if (auto it = m_windows.find(w); it != m_windows.end()) {
            effects->makeOpenGLContextCurrent();
            m_windows.erase(it);
        }
    }
}

void BlurEffect::slotWindowAdded(EffectWindow *w)
{
    SurfaceInterface *surf = w->surface();

    if (surf) {
        windowBlurChangedConnections[w] = connect(surf, &SurfaceInterface::blurChanged, this, [this, w]() {
            if (w) {
                updateBlurRegion(w);
            }
        });
        windowContrastChangedConnections[w] = connect(surf, &SurfaceInterface::contrastChanged, this, [this, w]() {
            if (w) {
                updateBlurRegion(w);
            }
        });
    }

    windowFrameGeometryChangedConnections[w] = connect(w, &EffectWindow::windowFrameGeometryChanged, this, [this,w]() {
        if (w) {
            updateBlurRegion(w);
        }
    });

    if (auto internal = w->internalWindow()) {
        internal->installEventFilter(this);
    }

    setupDecorationConnections(w);
    connect(w, &EffectWindow::windowDecorationChanged, this, [this, w]() {
        setupDecorationConnections(w);
        updateBlurRegion(w);
    });

    updateBlurRegion(w);
}

void BlurEffect::slotWindowDeleted(EffectWindow *w)
{
    if (auto it = m_windows.find(w); it != m_windows.end()) {
        effects->makeOpenGLContextCurrent();
        m_windows.erase(it);
    }
    if (auto it = windowBlurChangedConnections.find(w); it != windowBlurChangedConnections.end()) {
        disconnect(*it);
        windowBlurChangedConnections.erase(it);
    }
    if (auto it = windowContrastChangedConnections.find(w); it != windowContrastChangedConnections.end()) {
        disconnect(*it);
        windowContrastChangedConnections.erase(it);
    }
    if (auto it = windowFrameGeometryChangedConnections.find(w); it != windowFrameGeometryChangedConnections.end()) {
        disconnect(*it);
        windowFrameGeometryChangedConnections.erase(it);
    }
}

void BlurEffect::slotViewRemoved(KWin::RenderView *view)
{
    for (auto &[window, data] : m_windows) {
        if (auto it = data.render.find(view); it != data.render.end()) {
            effects->makeOpenGLContextCurrent();
            data.render.erase(it);
        }
    }
}

#if KWIN_BUILD_X11
void BlurEffect::slotPropertyNotify(EffectWindow *w, long atom)
{
    if (w && atom == net_wm_blur_region && net_wm_blur_region != XCB_ATOM_NONE) {
        updateBlurRegion(w);
    }
}
#endif

void BlurEffect::setupDecorationConnections(EffectWindow *w)
{
    if (!w->decoration()) {
        return;
    }

    connect(w->decoration(), &KDecoration3::Decoration::blurRegionChanged, this, [this, w]() {
        updateBlurRegion(w);
    });
}

bool BlurEffect::eventFilter(QObject *watched, QEvent *event)
{
    auto internal = qobject_cast<QWindow *>(watched);
    if (internal && event->type() == QEvent::DynamicPropertyChange) {
        QDynamicPropertyChangeEvent *pe = static_cast<QDynamicPropertyChangeEvent *>(event);
        if (pe->propertyName() == "kwin_blur") {
            if (auto w = effects->findWindow(internal)) {
                updateBlurRegion(w);
            }
        }
    }
    return false;
}

bool BlurEffect::enabledByDefault()
{
    const auto context = effects->openglContext();
    if (!context || context->isSoftwareRenderer()) {
        return false;
    }
    GLPlatform *gl = context->glPlatform();

    if (gl->isIntel() && gl->chipClass() < SandyBridge) {
        return false;
    }
    if (gl->isPanfrost() && gl->chipClass() <= MaliT8XX) {
        return false;
    }
    // The blur effect works, but is painfully slow (FPS < 5) on Mali and VideoCore
    if (gl->isLima() || gl->isVideoCore4() || gl->isVideoCore3D()) {
        return false;
    }
    return true;
}

bool BlurEffect::supported()
{
    return effects->isOpenGLCompositing();
}

bool BlurEffect::decorationSupportsBlurBehind(const EffectWindow *w) const
{
    return w->decoration() && !w->decoration()->blurRegion().isNull();
}

Region BlurEffect::decorationBlurRegion(const EffectWindow *w) const
{
    if (!decorationSupportsBlurBehind(w)) {
        return Region();
    }

    Region decorationRegion = Region(Rect(w->decoration()->rect().toAlignedRect())) - w->contentsRect().toRect();
    //! we return only blurred regions that belong to decoration region
    return decorationRegion.intersected(Region(w->decoration()->blurRegion()));
}

Region BlurEffect::blurRegion(EffectWindow *w) const
{
    Region region;

    if (auto it = m_windows.find(w); it != m_windows.end()) {
        const std::optional<Region> &content = it->second.content;
        const std::optional<Region> &frame = it->second.frame;
        if (content.has_value()) {
            if (content->isEmpty()) {
                // An empty region means that the blur effect should be enabled
                // for the whole window.
                region = Rect(w->contentsRect().toRect());
            } else {
                region = content->translated(w->contentsRect().topLeft().toPoint()) & w->contentsRect().toRect();
            }
            if (frame.has_value()) {
                region += frame.value();
            }
        } else if (frame.has_value()) {
            region = frame.value();
        }
    }

    return region;
}

void BlurEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    m_paintedDeviceArea = Region();
    m_currentDeviceBlur = Region();
    m_currentView = data.view;

    effects->prePaintScreen(data, presentTime);
}

void BlurEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime)
{
    effects->prePaintWindow(view, w, data, presentTime);

    // 1. Determine the area this window intends to blur
    const Region blurArea = view->mapToDeviceCoordinatesAligned(
        QRectF(blurRegion(w).boundingRect()).translated(w->pos())
    );

    if (!blurArea.isEmpty()) {
        // 2. FORCE TRANSPARENCY: Remove the blur area from the opaque region.
        // This prevents KWin from "occluding" (skipping) the rendering of
        // windows/wallpaper behind this specific area.
        data.deviceOpaque -= blurArea;

        // 3. EXPAND PAINT REGION: If we are blurring, we might need to
        // pull in pixels from slightly outside the immediate window
        // bounds for the refraction/dual-kawase offset.
        Region expandedBlur = blurArea;
        for (const Rect &rect : blurArea.rects()) {
            expandedBlur += rect.adjusted(-m_expandSize, -m_expandSize, m_expandSize, m_expandSize);
        }

        // Ensure the compositor actually paints the background we need to sample
        data.devicePaint += (expandedBlur - data.deviceOpaque);
    }

    // Carry over the blur "damage" to windows lower in the stack
    if (m_paintedDeviceArea.intersects(blurArea) || data.devicePaint.intersects(blurArea)) {
        data.devicePaint += blurArea;
    }

    m_currentDeviceBlur += blurArea;
    m_paintedDeviceArea -= data.deviceOpaque;
    m_paintedDeviceArea += data.devicePaint;
}

bool BlurEffect::shouldBlur(const EffectWindow *w, int mask, const WindowPaintData &data) const
{
    if (effects->activeFullScreenEffect() && !w->data(WindowForceBlurRole).toBool()) {
        return false;
    }

    if (w->isDesktop()) {
        return false;
    }

    const auto windowClass = w->window()->resourceClass();
    const auto resourceName = w->window()->resourceName();

    const auto matches = m_windowClasses.contains(windowClass) || m_windowClasses.contains(resourceName);

    if ((m_whitelist && !matches) || (!m_whitelist && matches)) {
        return false;
    }

    bool scaled = !qFuzzyCompare(data.xScale(), 1.0) && !qFuzzyCompare(data.yScale(), 1.0);
    bool translated = data.xTranslation() || data.yTranslation();

    if ((scaled || (translated || (mask & PAINT_WINDOW_TRANSFORMED))) && !w->data(WindowForceBlurRole).toBool()) {
        return false;
    }

    return true;
}

void BlurEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data)
{
    blur(renderTarget, viewport, w, mask, deviceRegion, data);

    // Draw the window over the blurred area
    effects->drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);
}

GLTexture *BlurEffect::ensureNoiseTexture()
{
    if (m_noiseStrength == 0) {
        return nullptr;
    }

    const qreal scale = std::max(1.0, QGuiApplication::primaryScreen()->logicalDotsPerInch() / 96.0);
    if (!m_noisePass.noiseTexture || m_noisePass.noiseTextureScale != scale || m_noisePass.noiseTextureStength != m_noiseStrength) {
        // Init randomness based on time
        std::srand((uint)QTime::currentTime().msec());

        QImage noiseImage(QSize(256, 256), QImage::Format_Grayscale8);

        for (int y = 0; y < noiseImage.height(); y++) {
            uint8_t *noiseImageLine = (uint8_t *)noiseImage.scanLine(y);

            for (int x = 0; x < noiseImage.width(); x++) {
                noiseImageLine[x] = std::rand() % m_noiseStrength;
            }
        }

        noiseImage = noiseImage.scaled(noiseImage.size() * scale);

        m_noisePass.noiseTexture = GLTexture::upload(noiseImage);
        if (!m_noisePass.noiseTexture) {
            return nullptr;
        }
        m_noisePass.noiseTexture->setFilter(GL_NEAREST);
        m_noisePass.noiseTexture->setWrapMode(GL_REPEAT);
        m_noisePass.noiseTextureScale = scale;
        m_noisePass.noiseTextureStength = m_noiseStrength;
    }

    return m_noisePass.noiseTexture.get();
}

void BlurEffect::blur(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data)
{
    auto it = m_windows.find(w);
    if (it == m_windows.end()) {
        return;
    }

    BlurEffectData &blurInfo = it->second;
    BlurRenderData &renderInfo = blurInfo.render[m_currentView];
    if (!shouldBlur(w, mask, data)) {
        return;
    }

    // Compute the effective blur shape. Note that if the window is transformed, so will be the blur shape.
    Region blurShape = blurRegion(w).translated(w->pos().toPoint());
    if (data.xScale() != 1 || data.yScale() != 1) {
        QPoint pt = blurShape.boundingRect().topLeft();
        Region scaledShape;
        for (const Rect &r : blurShape.rects()) {
            const QPointF topLeft(pt.x() + (r.x() - pt.x()) * data.xScale() + data.xTranslation(),
                                  pt.y() + (r.y() - pt.y()) * data.yScale() + data.yTranslation());
            const QPoint bottomRight(std::floor(topLeft.x() + r.width() * data.xScale()) - 1,
                                     std::floor(topLeft.y() + r.height() * data.yScale()) - 1);
            scaledShape += QRect(QPoint(std::floor(topLeft.x()), std::floor(topLeft.y())), bottomRight);
        }
        blurShape = scaledShape;
    } else if (data.xTranslation() || data.yTranslation()) {
        blurShape.translate(std::round(data.xTranslation()), std::round(data.yTranslation()));
    }

    const QRect backgroundRect = blurShape.boundingRect();
    const QRect scaledBackgroundRect = snapToPixelGrid(scaledRect(backgroundRect, viewport.scale()));
    const QRect deviceBackgroundRect = snapToPixelGrid(viewport.mapToDeviceCoordinates(backgroundRect));
    const auto opacity = data.opacity();

    // Get the effective shape that will be actually blurred. It's possible that all of it will be clipped.
    QList<RectF> effectiveShape;
    effectiveShape.reserve(blurShape.rects().size());
    if (deviceRegion != Region::infinite()) {
        for (const Rect &clipRect : deviceRegion.rects()) {
            const RectF deviceClipRect = clipRect.translated(-deviceBackgroundRect.topLeft());
            for (const Rect &shapeRect : blurShape.rects()) {
                const RectF deviceShapeRect = shapeRect.translated(-backgroundRect.topLeft()).scaled(viewport.scale()).rounded();
                if (const QRectF intersected = deviceClipRect.intersected(deviceShapeRect); !intersected.isEmpty()) {
                    effectiveShape.append(intersected);
                }
            }
        }
    } else {
        for (const Rect &rect : blurShape.rects()) {
            effectiveShape.append(rect.translated(-backgroundRect.topLeft()).scaled(viewport.scale()).rounded());
        }
    }
    if (effectiveShape.isEmpty()) {
        return;
    }

    // Maybe reallocate offscreen render targets. Keep in mind that the first one contains
    // original background behind the window, it's not blurred.
    GLenum textureFormat = GL_RGBA8;
    if (renderTarget.texture()) {
        textureFormat = renderTarget.texture()->internalFormat();
    }

    if (renderInfo.framebuffers.size() != (m_iterationCount + 1) || renderInfo.textures[0]->size() != backgroundRect.size() || renderInfo.textures[0]->internalFormat() != textureFormat) {
        renderInfo.framebuffers.clear();
        renderInfo.textures.clear();

        glClearColor(0, 0, 0, 0);
        for (size_t i = 0; i <= m_iterationCount; ++i) {
            auto texture = GLTexture::allocate(textureFormat, backgroundRect.size() / (1 << i));
            if (!texture) {
                qCWarning(KWIN_BLUR) << "Failed to allocate an offscreen texture";
                return;
            }
            texture->setFilter(GL_LINEAR);
            texture->setWrapMode(GL_CLAMP_TO_EDGE);

            auto framebuffer = std::make_unique<GLFramebuffer>(texture.get());
            if (!framebuffer->valid()) {
                qCWarning(KWIN_BLUR) << "Failed to create an offscreen framebuffer";
                return;
            }
            EglContext::currentContext()->pushFramebuffer(framebuffer.get());
            glClear(GL_COLOR_BUFFER_BIT);
            EglContext::currentContext()->popFramebuffer();
            renderInfo.textures.push_back(std::move(texture));
            renderInfo.framebuffers.push_back(std::move(framebuffer));
        }
    }

    // Fetch the pixels behind the shape that is going to be blurred.
    const Region dirtyRegion = viewport.mapFromDeviceCoordinatesContained(deviceRegion) & backgroundRect;
    for (const Rect &dirtyRect : dirtyRegion.rects()) {
        renderInfo.framebuffers[0]->blitFromRenderTarget(renderTarget, viewport, dirtyRect, dirtyRect.translated(-backgroundRect.topLeft()));
    }

    // Upload the geometry: the first 6 vertices are used when downsampling and upsampling offscreen,
    // the remaining vertices are used when rendering on the screen.
    GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
    vbo->reset();
    vbo->setAttribLayout(std::span(GLVertexBuffer::GLVertex2DLayout), sizeof(GLVertex2D));

    const int vertexCount = effectiveShape.size() * 6;
    if (auto result = vbo->map<GLVertex2D>(6 + vertexCount)) {
        auto map = *result;

        size_t vboIndex = 0;

        // The geometry that will be blurred offscreen, in logical pixels.
        {
            const QRectF localRect = QRectF(0, 0, backgroundRect.width(), backgroundRect.height());

            const float x0 = localRect.left();
            const float y0 = localRect.top();
            const float x1 = localRect.right();
            const float y1 = localRect.bottom();

            const float u0 = x0 / backgroundRect.width();
            const float v0 = 1.0f - y0 / backgroundRect.height();
            const float u1 = x1 / backgroundRect.width();
            const float v1 = 1.0f - y1 / backgroundRect.height();

            // first triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y1),
                .texcoord = QVector2D(u0, v1),
            };

            // second triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y0),
                .texcoord = QVector2D(u1, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
        }

        // The geometry that will be painted on screen, in device pixels.
        for (const QRectF &rect : effectiveShape) {
            const float x0 = rect.left();
            const float y0 = rect.top();
            const float x1 = rect.right();
            const float y1 = rect.bottom();

            const float u0 = x0 / scaledBackgroundRect.width();
            const float v0 = 1.0f - y0 / scaledBackgroundRect.height();
            const float u1 = x1 / scaledBackgroundRect.width();
            const float v1 = 1.0f - y1 / scaledBackgroundRect.height();

            // first triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y1),
                .texcoord = QVector2D(u0, v1),
            };

            // second triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y0),
                .texcoord = QVector2D(u1, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
        }

        vbo->unmap();
    } else {
        qCWarning(KWIN_BLUR) << "Failed to map vertex buffer";
        return;
    }

    vbo->bindArrays();

    // The downsample pass of the dual Kawase algorithm: the background will be scaled down 50% every iteration.
    {
        ShaderManager::instance()->pushShader(m_downsamplePass.shader.get());

        QMatrix4x4 projectionMatrix;
        projectionMatrix.ortho(QRectF(0.0, 0.0, backgroundRect.width(), backgroundRect.height()));

        m_downsamplePass.shader->setUniform(m_downsamplePass.mvpMatrixLocation, projectionMatrix);
        m_downsamplePass.shader->setUniform(m_downsamplePass.offsetLocation, float(m_offset));

        for (size_t i = 1; i < renderInfo.framebuffers.size(); ++i) {
            const auto &read = renderInfo.framebuffers[i - 1];
            const auto &draw = renderInfo.framebuffers[i];

            const QVector2D halfpixel(0.5 / read->colorAttachment()->width(),
                                      0.5 / read->colorAttachment()->height());
            m_downsamplePass.shader->setUniform(m_downsamplePass.halfpixelLocation, halfpixel);

            read->colorAttachment()->bind();

            GLFramebuffer::pushFramebuffer(draw.get());
            vbo->draw(GL_TRIANGLES, 0, 6);
        }

        ShaderManager::instance()->popShader();
    }

    // The upsample pass of the dual Kawase algorithm: the background will be scaled up 200% every iteration.
    {
        ShaderManager::instance()->pushShader(m_upsamplePass.shader.get());

        QMatrix4x4 projectionMatrix;
        projectionMatrix.ortho(QRectF(0.0, 0.0, backgroundRect.width(), backgroundRect.height()));

        m_upsamplePass.shader->setUniform(m_upsamplePass.mvpMatrixLocation, projectionMatrix);
        m_upsamplePass.shader->setUniform(m_upsamplePass.offsetLocation, float(m_offset));

        for (size_t i = renderInfo.framebuffers.size() - 1; i > 1; --i) {
            GLFramebuffer::popFramebuffer();
            const auto &read = renderInfo.framebuffers[i];

            const QVector2D halfpixel(0.5 / read->colorAttachment()->width(),
                                      0.5 / read->colorAttachment()->height());
            m_upsamplePass.shader->setUniform(m_upsamplePass.halfpixelLocation, halfpixel);

            read->colorAttachment()->bind();

            vbo->draw(GL_TRIANGLES, 0, 6);
        }

        ShaderManager::instance()->popShader();
    }

    const QMatrix4x4 &colorMatrix = m_colorMatrix;
    const float modulation = opacity * opacity;

    BorderRadius cornerRadius = w->window()->borderRadius();

    float topCornerRadius = 0.0;
    float bottomCornerRadius = 0.0;
    if (w->isOnScreenDisplay() || w->isTooltip()) {
        topCornerRadius = m_settings.roundedCorners.windowTopRadius;
        bottomCornerRadius = m_settings.roundedCorners.windowBottomRadius;
    } else if (w->isDock()) {
        topCornerRadius = m_settings.roundedCorners.dockRadius;
        bottomCornerRadius = m_settings.roundedCorners.dockRadius;
    } else if (w->isMenu() || w->isDropdownMenu() || w->isPopupMenu() || w->isPopupWindow()) {
        topCornerRadius = m_settings.roundedCorners.menuRadius;
        bottomCornerRadius = m_settings.roundedCorners.menuRadius;
    } else if (!w->isFullScreen() || m_settings.roundedCorners.roundMaximized) {
        topCornerRadius = m_settings.roundedCorners.windowTopRadius;
        bottomCornerRadius = m_settings.roundedCorners.windowBottomRadius;
    }

    bool isOverRounded = false;
    if (topCornerRadius > 0 || bottomCornerRadius > 0) {
        const QRectF frame = w->frameGeometry();
        const float winWidth = frame.width();
        const float winHeight = frame.height();

        isOverRounded = (topCornerRadius + bottomCornerRadius) > winHeight ||
            (topCornerRadius * 2) > winWidth;

        if (isOverRounded) {
            if (w->isDock()) {
                topCornerRadius = 0;
                bottomCornerRadius = 0;
            } else {
                float minRadius = std::min(winWidth, winHeight) / 2.0;
                topCornerRadius = minRadius;
                bottomCornerRadius = minRadius;
            }
        }

        cornerRadius = BorderRadius(
            topCornerRadius, // top left
            topCornerRadius, // top right
            bottomCornerRadius, // bottom right
            bottomCornerRadius // bottom left
        );
    }


    ShaderManager::instance()->pushShader(m_roundedOnscreenPass.shader.get());

    QMatrix4x4 projectionMatrix = viewport.projectionMatrix();
    projectionMatrix.translate(scaledBackgroundRect.x(), scaledBackgroundRect.y());

    GLFramebuffer::popFramebuffer();
    const auto &read = renderInfo.framebuffers[1];

    const QVector2D halfpixel(0.5 / read->colorAttachment()->width(),
                              0.5 / read->colorAttachment()->height());

    const QRectF transformedRect = QRectF{
        w->frameGeometry().x() + data.xTranslation(),
        w->frameGeometry().y() + data.yTranslation(),
        w->frameGeometry().width() * data.xScale(),
        w->frameGeometry().height() * data.yScale(),
    };
    const QRectF nativeBox = snapToPixelGridF(scaledRect(transformedRect, viewport.scale()))
                                 .translated(-scaledBackgroundRect.topLeft());
    const BorderRadius nativeCornerRadius = cornerRadius.scaled(viewport.scale()).rounded();

    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.mvpMatrixLocation, projectionMatrix);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.colorMatrixLocation, colorMatrix);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.halfpixelLocation, halfpixel);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.offsetLocation, float(m_offset));
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.boxLocation, QVector4D(nativeBox.x() + nativeBox.width() * 0.5, nativeBox.y() + nativeBox.height() * 0.5, nativeBox.width() * 0.5, nativeBox.height() * 0.5));
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.cornerRadiusLocation, nativeCornerRadius.toVector());
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.opacityLocation, modulation);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.blurSizeLocation, QVector2D(nativeBox.width(), nativeBox.height()));
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.edgeSizePixelsLocation, m_settings.refraction.edgeSizePixels);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.refractionStrengthLocation, m_settings.refraction.refractionStrength);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.refractionNormalPowLocation, m_settings.refraction.refractionNormalPow);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.refractionRGBFringingLocation, m_settings.refraction.refractionRGBFringing);

    QColor tint(m_settings.general.tintColor);
    QVector3D tintVec(tint.redF(), tint.greenF(), tint.blueF());
    float tintStrength = w->isDock() && m_settings.general.excludeDocks ? 0.0f : static_cast<float>(tint.alphaF());
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.tintColorLocation, tintVec);
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.tintStrengthLocation, tintStrength);

    QColor glow(m_settings.general.glowColor);
    QVector3D glowVec(glow.redF(), glow.greenF(), glow.blueF());
    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.glowColorLocation, glowVec);
    if (isOverRounded && w->isDock()) {
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.glowStrengthLocation, 0.0);
    } else {
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.glowStrengthLocation, static_cast<float>(glow.alphaF()));
    }

    m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.edgeLightingLocation, m_settings.general.edgeLighting);

    read->colorAttachment()->bind();

    glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    vbo->draw(GL_TRIANGLES, 6, vertexCount);

    glDisable(GL_BLEND);

    ShaderManager::instance()->popShader();

    if (m_noiseStrength > 0) {
        // Apply an additive noise onto the blurred image. The noise is useful to mask banding
        // artifacts, which often happens due to the smooth color transitions in the blurred image.

        glEnable(GL_BLEND);
        if (opacity < 1.0) {
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_ONE, GL_ONE);
        }

        if (GLTexture *noiseTexture = ensureNoiseTexture()) {
            ShaderManager::instance()->pushShader(m_noisePass.shader.get());

            QMatrix4x4 projectionMatrix = viewport.projectionMatrix();
            projectionMatrix.translate(scaledBackgroundRect.x(), scaledBackgroundRect.y());

            m_noisePass.shader->setUniform(m_noisePass.mvpMatrixLocation, projectionMatrix);
            m_noisePass.shader->setUniform(m_noisePass.noiseTextureSizeLocation, QVector2D(noiseTexture->width(), noiseTexture->height()));

            noiseTexture->bind();

            vbo->draw(GL_TRIANGLES, 6, vertexCount);

            ShaderManager::instance()->popShader();
        }

        glDisable(GL_BLEND);
    }

    vbo->unbindArrays();
}

bool BlurEffect::isActive() const
{
    return m_valid && !effects->isScreenLocked();
}

bool BlurEffect::blocksDirectScanout() const
{
    return false;
}

} // namespace KWin

#include "moc_blur.cpp"
