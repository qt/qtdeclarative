// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qquickpixmap_p.h>
#include <QtQuick/private/qsgadaptationlayer_p.h>
#include <QtQuick/private/qsginternaltextnode_p.h>

#include <QGuiApplication>
#include <QScreen>
#include <QQuickWindow>

#include <private/qqmlglobal_p.h>

#include <QtQuick/private/qsgtexture_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/private/qabstractanimation_p.h>

#include <private/qobject_p.h>
#include <qmutex.h>

/*
    Comments about this class from Gunnar:

    The QSGContext class is right now two things.. The first is the
    adaptation layer and central storage ground for all the things
    in the scene graph, like textures and materials. This part really
    belongs inside the scene graph coreapi.

    The other part is the QML adaptation classes, like how to implement
    rectangle nodes. This is not part of the scene graph core API, but
    more part of the QML adaptation of scene graph.

    If we ever move the scene graph core API into its own thing, this class
    needs to be split in two. Right now its one because we're lazy when it comes
    to defining plugin interfaces..
*/

QT_BEGIN_NAMESPACE

// Used for very high-level info about the renderering and gl context
// Includes GL_VERSION, type of render loop, atlas size, etc.
Q_LOGGING_CATEGORY(QSG_LOG_INFO,                "qt.scenegraph.general")

// Used to debug the renderloop logic. Primarily useful for platform integrators
// and when investigating the render loop logic.
Q_LOGGING_CATEGORY(QSG_LOG_RENDERLOOP,          "qt.scenegraph.renderloop")


// GLSL shader compilation
Q_LOGGING_CATEGORY(QSG_LOG_TIME_COMPILATION,    "qt.scenegraph.time.compilation")

// polish, animations, sync, render and swap in the render loop
Q_LOGGING_CATEGORY(QSG_LOG_TIME_RENDERLOOP,     "qt.scenegraph.time.renderloop")

// Texture uploads and swizzling
Q_LOGGING_CATEGORY(QSG_LOG_TIME_TEXTURE,        "qt.scenegraph.time.texture")

// Glyph preparation (only for distance fields atm)
Q_LOGGING_CATEGORY(QSG_LOG_TIME_GLYPH,          "qt.scenegraph.time.glyph")

// Timing inside the renderer base class
Q_LOGGING_CATEGORY(QSG_LOG_TIME_RENDERER,       "qt.scenegraph.time.renderer")

// Leak checks
Q_LOGGING_CATEGORY(lcQsgLeak,                   "qt.scenegraph.leaks")

// Applicable for render loops that install their own animation driver, such as
// the 'threaded' loop. This env.var. is documented in the scenegraph docs.
DEFINE_BOOL_CONFIG_OPTION(useElapsedTimerBasedAnimationDriver, QSG_USE_SIMPLE_ANIMATION_DRIVER);

bool qsg_useConsistentTiming()
{
    int use = -1;
    if (use < 0) {
        use = !qEnvironmentVariableIsEmpty("QSG_FIXED_ANIMATION_STEP") && qgetenv("QSG_FIXED_ANIMATION_STEP") != "no"
            ? 1 : 0;
        qCDebug(QSG_LOG_INFO, "Using %s", bool(use) ? "fixed animation steps" : "sg animation driver");
    }
    return bool(use);
}

class QSGAnimationDriver : public QAnimationDriver
{
public:
    QSGAnimationDriver(QObject *parent)
        : QAnimationDriver(parent)
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            qreal refreshRate = screen->refreshRate();
            // To work around that some platforms wrongfully return 0 or something
            // bogus for the refresh rate.
            if (refreshRate < 1)
                refreshRate = 60;
            m_vsync = 1000.0f / float(refreshRate);
        } else {
            m_vsync = 16.67f;
        }
    }

    float vsyncInterval() const { return m_vsync; }

    virtual bool isVSyncDependent() const = 0;

protected:
    float m_vsync = 0;
};

// default as in default for the threaded render loop
class QSGDefaultAnimationDriver : public QSGAnimationDriver
{
    Q_OBJECT
public:
    enum Mode {
        VSyncMode,
        TimerMode
    };

    QSGDefaultAnimationDriver(QObject *parent)
        : QSGAnimationDriver(parent)
        , m_time(0)
        , m_mode(VSyncMode)
        , m_lag(0)
        , m_bad(0)
        , m_good(0)
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen && !qsg_useConsistentTiming()) {
            if (m_vsync <= 0)
                m_mode = TimerMode;
        } else {
            m_mode = TimerMode;
            if (qsg_useConsistentTiming())
                QUnifiedTimer::instance(true)->setConsistentTiming(true);
        }
        if (m_mode == VSyncMode)
            qCDebug(QSG_LOG_INFO, "Animation Driver: using vsync: %.2f ms", m_vsync);
        else
            qCDebug(QSG_LOG_INFO, "Animation Driver: using walltime");
    }

    void start() override
    {
        m_time = 0;
        m_timer.start();
        m_wallTime.start();
        QAnimationDriver::start();
    }

    qint64 elapsed() const override
    {
        return m_mode == VSyncMode
                ? qint64(m_time)
                : qint64(m_time) + m_wallTime.elapsed();
    }

    void advance() override
    {
        qint64 delta = m_timer.restart();

        if (m_mode == VSyncMode) {
            // If a frame is skipped, either because rendering was slow or because
            // the QML was slow, we accept it and continue advancing with a single
            // vsync tick. The reason for this is that by the time we notice this
            // on the GUI thread, the temporal distortion has already gone to screen
            // and by catching up, we will introduce a second distortion which will
            // worse. We accept that the animation time falls behind wall time because
            // it comes out looking better.
            // Only when multiple bad frames are hit in a row, do we consider
            // switching. A few really bad frames and we switch right away. For frames
            // just above the vsync delta, we tolerate a bit more since a buffered
            // driver can have vsync deltas on the form: 4, 21, 21, 2, 23, 16, and
            // still manage to put the frames to screen at 16 ms intervals. In addition
            // to that, we tolerate a 25% margin of error on the value of m_vsync
            // reported from the system as this value is often not precise.

            m_time += m_vsync;

            if (delta > m_vsync * 1.25f) {
                m_lag += (delta / m_vsync);
                m_bad++;
               // We tolerate one bad frame without resorting to timer based. This is
                // done to cope with a slow loader frame followed by smooth animation.
                // However, on the second frame with massive lag, we switch.
                if (m_lag > 10 && m_bad > 2) {
                    m_mode = TimerMode;
                    qCDebug(QSG_LOG_INFO, "animation driver switched to timer mode");
                    m_wallTime.start();
                }
            } else {
                m_lag = 0;
                m_bad = 0;
            }

        } else {
            if (delta < 1.25f * m_vsync) {
                ++m_good;
            } else {
                m_good = 0;
            }

            // We've been solid for a while, switch back to vsync mode. Tolerance
            // for switching back is lower than switching to timer mode, as we
            // want to stay in vsync mode as much as possible.
            if (m_good > 10 && !qsg_useConsistentTiming()) {
                m_time = elapsed();
                m_mode = VSyncMode;
                m_bad = 0;
                m_lag = 0;
                qCDebug(QSG_LOG_INFO, "animation driver switched to vsync mode");
            }
        }

        advanceAnimation();
    }

    bool isVSyncDependent() const override
    {
        return true;
    }

    double m_time;
    Mode m_mode;
    QElapsedTimer m_timer;
    QElapsedTimer m_wallTime;
    float m_lag;
    int m_bad;
    int m_good;
};

// Advance based on QElapsedTimer. (so like the TimerMode of QSGDefaultAnimationDriver)
// Does not depend on vsync-based throttling.
//
// NB this is not the same as not installing a QAnimationDriver: the built-in
// approach in QtCore is to rely on 16 ms timer events which are potentially a
// lot less accurate.
//
// This has the benefits of:
// - not needing any of the infrastructure for falling back to a
//   QTimer when there are multiple windows,
// - needing no heuristics trying determine if vsync-based throttling
//   is missing or broken,
// - being compatible with any kind of temporal drifts in vsync throttling
//   which is reportedly happening in various environments and platforms
//   still,
// - not being tied to the primary screen's refresh rate, i.e. this is
//   correct even if the window is on some secondary screen with a
//   different refresh rate,
// - not having to worry about the potential effects of variable refresh
//   rate solutions,
// - render thread animators work correctly regardless of vsync.
//
// On the downside, some animations might appear less smooth (compared to the
// ideal single window case of QSGDefaultAnimationDriver).
//
class QSGElapsedTimerAnimationDriver : public QSGAnimationDriver
{
public:
    QSGElapsedTimerAnimationDriver(QObject *parent)
        : QSGAnimationDriver(parent)
    {
        qCDebug(QSG_LOG_INFO, "Animation Driver: using QElapsedTimer, thread %p %s",
                QThread::currentThread(),
                QThread::currentThread() == qGuiApp->thread() ? "(gui/main thread)" : "(render thread)");
    }

    void start() override
    {
        m_wallTime.start();
        QAnimationDriver::start();
    }

    qint64 elapsed() const override
    {
        return m_wallTime.elapsed();
    }

    void advance() override
    {
        advanceAnimation();
    }

    bool isVSyncDependent() const override
    {
        return false;
    }

private:
    QElapsedTimer m_wallTime;
};

QSGRenderContext::FontKey::FontKey(const QRawFont &font, int quality)
{
    QFontEngine *fe = QRawFontPrivate::get(font)->fontEngine;
    if (fe != nullptr)
        faceId = fe->faceId();
    style = font.style();
    weight = font.weight();
    renderTypeQuality = quality;
    if (faceId.filename.isEmpty()) {
        familyName = font.familyName();
        styleName = font.styleName();
    }
}

/*!
    \class QSGContext

    \brief The QSGContext holds the scene graph entry points for one QML engine.

    The context is not ready for use until it has a QRhi. Once that happens,
    the scene graph population can start.

    \internal
 */

QSGContext::QSGContext(QObject *parent) :
    QObject(parent)
{
}

QSGContext::~QSGContext()
{
}

void QSGContext::renderContextInitialized(QSGRenderContext *)
{
}

void QSGContext::renderContextInvalidated(QSGRenderContext *)
{
}


/*!
    Convenience factory function for creating a colored rectangle with the given geometry.
 */
QSGInternalRectangleNode *QSGContext::createInternalRectangleNode(const QRectF &rect, const QColor &c)
{
    QSGInternalRectangleNode *node = createInternalRectangleNode();
    node->setRect(rect);
    node->setColor(c);
    node->update();
    return node;
}

QSGInternalTextNode *QSGContext::createInternalTextNode(QSGRenderContext *renderContext)
{
    return new QSGInternalTextNode(renderContext);
}

QSGTextNode *QSGContext::createTextNode(QSGRenderContext *renderContext)
{
    return createInternalTextNode(renderContext);
}

/*!
    Creates a new shader effect helper instance. This function is called on the
    GUI thread, unlike the others. This is necessary in order to provide
    adaptable, backend-specific shader effect functionality to the GUI thread too.
 */
QSGGuiThreadShaderEffectManager *QSGContext::createGuiThreadShaderEffectManager()
{
    return nullptr;
}

/*!
    Creates a new shader effect node. The default of returning nullptr is
    valid as long as the backend does not claim SupportsShaderEffectNode or
    ignoring ShaderEffect elements is acceptable.
 */
QSGShaderEffectNode *QSGContext::createShaderEffectNode(QSGRenderContext *)
{
    return nullptr;
}

/*!
    Creates a new animation driver.
 */
QAnimationDriver *QSGContext::createAnimationDriver(QObject *parent)
{
    if (useElapsedTimerBasedAnimationDriver())
        return new QSGElapsedTimerAnimationDriver(parent);

    return new QSGDefaultAnimationDriver(parent);
}

/*!
    \return the vsync rate (such as, 16.68 ms or similar), if applicable, for
    the \a driver that was created by createAnimationDriver().
 */
float QSGContext::vsyncIntervalForAnimationDriver(QAnimationDriver *driver)
{
    return static_cast<QSGAnimationDriver *>(driver)->vsyncInterval();
}

/*!
    \return true if \a driver relies on vsync-based throttling in some form.
 */
bool QSGContext::isVSyncDependent(QAnimationDriver *driver)
{
    return static_cast<QSGAnimationDriver *>(driver)->isVSyncDependent();
}

QSize QSGContext::minimumFBOSize() const
{
    return QSize(1, 1);
}

/*!
    Returns a pointer to the (presumably) global renderer interface.

    \note This function may be called on the GUI thread in order to get access
    to QSGRendererInterface::graphicsApi() and other getters.

    \note it is expected that the simple queries (graphicsApi, shaderType,
    etc.) are available regardless of the render context validity (i.e.
    scenegraph status). This does not apply to engine-specific getters like
    getResource(). In the end this means that this function must always return
    a valid object in subclasses, even when renderContext->isValid() is false.
    The typical pattern is to implement the QSGRendererInterface in the
    QSGContext or QSGRenderContext subclass itself, whichever is more suitable.
 */
QSGRendererInterface *QSGContext::rendererInterface(QSGRenderContext *renderContext)
{
    Q_UNUSED(renderContext);
    qWarning("QSGRendererInterface not implemented");
    return nullptr;
}

QSGRenderContext::QSGRenderContext(QSGContext *context)
    : m_sg(context)
{
}

QSGRenderContext::~QSGRenderContext()
{
}

void QSGRenderContext::initialize(const InitParams *params)
{
    Q_UNUSED(params);
}

void QSGRenderContext::invalidate()
{
}

void QSGRenderContext::prepareSync(qreal devicePixelRatio,
                                   QRhiCommandBuffer *cb,
                                   const QQuickGraphicsConfiguration &config)
{
    Q_UNUSED(devicePixelRatio);
    Q_UNUSED(cb);
    Q_UNUSED(config);
}

void QSGRenderContext::beginNextFrame(QSGRenderer *renderer, const QSGRenderTarget &renderTarget,
                                      RenderPassCallback mainPassRecordingStart,
                                      RenderPassCallback mainPassRecordingEnd,
                                      void *callbackUserData)
{
    renderer->setRenderTarget(renderTarget);
    Q_UNUSED(mainPassRecordingStart);
    Q_UNUSED(mainPassRecordingEnd);
    Q_UNUSED(callbackUserData);
}

void QSGRenderContext::endNextFrame(QSGRenderer *renderer)
{
    Q_UNUSED(renderer);
}

void QSGRenderContext::endSync()
{
    qDeleteAll(m_texturesToDelete);
    m_texturesToDelete.clear();
}

/*!
    Do necessary preprocessing before the frame
*/
void QSGRenderContext::preprocess()
{
}

/*!
    Factory function for curve atlases that can be used to provide geometry for the curve
    renderer for a given font.
*/
QSGCurveGlyphAtlas *QSGRenderContext::curveGlyphAtlas(const QRawFont &font)
{
    Q_UNUSED(font);
    return nullptr;
}

/*!
    Factory function for scene graph backends of the distance-field glyph cache.
 */
QSGDistanceFieldGlyphCache *QSGRenderContext::distanceFieldGlyphCache(const QRawFont &, int)
{
    return nullptr;
}

void QSGRenderContext::invalidateGlyphCaches()
{

}

void QSGRenderContext::registerFontengineForCleanup(QFontEngine *engine)
{
    engine->ref.ref();
    m_fontEnginesToClean[engine]++;
}

void QSGRenderContext::unregisterFontengineForCleanup(QFontEngine *engine)
{
    m_fontEnginesToClean[engine]--;
    Q_ASSERT(m_fontEnginesToClean.value(engine) >= 0);
}

QRhi *QSGRenderContext::rhi() const
{
    return nullptr;
}

/*!
    Factory function for the scene graph renderers.

    The renderers are used for the toplevel renderer and once for every
    QQuickShaderEffectSource used in the QML scene.
 */

QSGTexture *QSGRenderContext::textureForFactory(QQuickTextureFactory *factory, QQuickWindow *window)
{
    if (!factory)
        return nullptr;

    m_mutex.lock();
    QSGTexture *texture = m_textures.value(factory);
    m_mutex.unlock();

    if (!texture) {
        texture = factory->createTexture(window);

        m_mutex.lock();
        m_textures.insert(factory, texture);
        m_mutex.unlock();

        connect(factory, SIGNAL(destroyed(QObject*)), this, SLOT(textureFactoryDestroyed(QObject*)), Qt::DirectConnection);
    }
    return texture;
}

void QSGRenderContext::textureFactoryDestroyed(QObject *o)
{
    m_mutex.lock();
    m_texturesToDelete << m_textures.take(o);
    m_mutex.unlock();
}

/*!
    Return the texture corresponding to a texture factory.

    This may optionally manipulate the texture in some way; for example by returning
    an atlased texture.

    This function is not a replacement for textureForFactory; both should be used
    for a single texture (this might atlas, while the other might cache).
*/
QSGTexture *QSGRenderContext::compressedTextureForFactory(const QSGCompressedTextureFactory *) const
{
    return nullptr;
}

QT_END_NAMESPACE

#include "qsgcontext.moc"
#include "moc_qsgcontext_p.cpp"
