/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickcontext2dtexture_p.h"
#include "qquickcontext2dtile_p.h"
#include "qquickcanvasitem_p.h"
#include <private/qquickitem_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include "qquickcontext2dcommandbuffer_p.h"
#include <QOpenGLPaintDevice>

#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QtCore/QThread>

#define QT_MINIMUM_FBO_SIZE 64

static inline int qt_next_power_of_two(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}


Q_GLOBAL_STATIC(QThread, globalCanvasThreadRenderInstance)


QQuickContext2DTexture::QQuickContext2DTexture()
    : QSGDynamicTexture()
    , m_context(0)
    , m_item(0)
    , m_canvasSize(QSize(1, 1))
    , m_tileSize(QSize(1, 1))
    , m_canvasWindow(QRect(0, 0, 1, 1))
    , m_dirtyCanvas(false)
    , m_dirtyTexture(false)
    , m_threadRendering(false)
    , m_smooth(false)
    , m_tiledCanvas(false)
    , m_doGrabImage(false)
    , m_painting(false)
{
}

QQuickContext2DTexture::~QQuickContext2DTexture()
{
   clearTiles();
}

QSize QQuickContext2DTexture::textureSize() const
{
    return m_canvasWindow.size();
}

void QQuickContext2DTexture::markDirtyTexture()
{
    lock();
    m_dirtyTexture = true;
    unlock();
    emit textureChanged();
}

bool QQuickContext2DTexture::setCanvasSize(const QSize &size)
{
    if (m_canvasSize != size) {
        m_canvasSize = size;
        m_dirtyCanvas = true;
        return true;
    }
    return false;
}

bool QQuickContext2DTexture::setTileSize(const QSize &size)
{
    if (m_tileSize != size) {
        m_tileSize = size;
        m_dirtyCanvas = true;
        return true;
    }
    return false;
}

void QQuickContext2DTexture::setSmooth(bool smooth)
{
    m_smooth = smooth;
}

void QQuickContext2DTexture::setItem(QQuickCanvasItem* item)
{
    if (!item) {
        lock();
        m_item = 0;
        m_context = 0;
        unlock();
        wake();
    } else if (m_item != item) {
        lock();
        m_item = item;
        m_context = item->context();
        m_state = m_context->state;
        unlock();
        connect(this, SIGNAL(textureChanged()), m_item, SIGNAL(painted()));
    }
}

bool QQuickContext2DTexture::setCanvasWindow(const QRect& r)
{
    if (m_canvasWindow != r) {
        m_canvasWindow = r;
        return true;
    }
    return false;
}

bool QQuickContext2DTexture::setDirtyRect(const QRect &r)
{
    bool doDirty = false;
    if (m_tiledCanvas) {
        foreach (QQuickContext2DTile* t, m_tiles) {
            bool dirty = t->rect().intersected(r).isValid();
            t->markDirty(dirty);
            if (dirty)
                doDirty = true;
        }
    } else {
        doDirty = m_canvasWindow.intersected(r).isValid();
    }
    return doDirty;
}

void QQuickContext2DTexture::canvasChanged(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth)
{
    lock();

    QSize ts = tileSize;
    if (ts.width() > canvasSize.width())
        ts.setWidth(canvasSize.width());

    if (ts.height() > canvasSize.height())
        ts.setHeight(canvasSize.height());

    setCanvasSize(canvasSize);
    setTileSize(ts);

    if (canvasSize == canvasWindow.size()) {
        m_tiledCanvas = false;
        m_dirtyCanvas = false;
    } else {
        m_tiledCanvas = true;
    }

    bool doDirty = false;
    if (dirtyRect.isValid())
        doDirty = setDirtyRect(dirtyRect);

    bool windowChanged = setCanvasWindow(canvasWindow);
    if (windowChanged || doDirty) {
        if (m_threadRendering)
            QMetaObject::invokeMethod(this, "paint", Qt::QueuedConnection);
        else if (supportDirectRendering()) {
           QMetaObject::invokeMethod(this, "paint", Qt::DirectConnection);
        }
    }

    setSmooth(smooth);
    unlock();
}

void QQuickContext2DTexture::paintWithoutTiles()
{
    QQuickContext2DCommandBuffer* ccb = m_context->buffer();

    if (ccb->isEmpty() && m_threadRendering && !m_doGrabImage) {
        lock();
        if (m_item)
            QMetaObject::invokeMethod(m_item, "_doPainting", Qt::QueuedConnection, Q_ARG(QRectF, QRectF(0, 0, m_canvasSize.width(), m_canvasSize.height())));
        wait();
        unlock();
    }
    if (ccb->isEmpty()) {
        return;
    }

    QPaintDevice* device = beginPainting();
    if (!device) {
        endPainting();
        return;
    }

    QPainter p;
    p.begin(device);
    if (m_smooth)
        p.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing
                               | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    else
        p.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing
                                 | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, false);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    ccb->replay(&p, m_state);

    ccb->clear();
    markDirtyTexture();
    endPainting();
}

bool QQuickContext2DTexture::canvasDestroyed()
{
    bool noCanvas = false;
    lock();
    noCanvas = m_item == 0;
    unlock();
    return noCanvas;
}

void QQuickContext2DTexture::paint()
{
    if (canvasDestroyed())
        return;

    if (!m_tiledCanvas) {
        paintWithoutTiles();
    } else {
        lock();
        QRect tiledRegion = createTiles(m_canvasWindow.intersected(QRect(QPoint(0, 0), m_canvasSize)));
        unlock();

        if (!tiledRegion.isEmpty()) {
            if (m_threadRendering && !m_doGrabImage) {
                QRect dirtyRect;

                lock();
                foreach (QQuickContext2DTile* tile, m_tiles) {
                    if (tile->dirty()) {
                        if (dirtyRect.isEmpty())
                            dirtyRect = tile->rect();
                        else
                            dirtyRect |= tile->rect();
                    }
                }
                unlock();

                if (dirtyRect.isValid()) {
                    lock();
                    if (m_item)
                        QMetaObject::invokeMethod(m_item, "_doPainting", Qt::QueuedConnection, Q_ARG(QRectF, dirtyRect));
                    wait();
                    unlock();
                }
            }

            if (beginPainting()) {
                QQuickContext2D::State oldState = m_state;
                QQuickContext2DCommandBuffer* ccb = m_context->buffer();
                foreach (QQuickContext2DTile* tile, m_tiles) {
                    bool dirtyTile = false, dirtyCanvas = false, smooth = false;

                    lock();
                    dirtyTile = tile->dirty();
                    smooth = m_smooth;
                    dirtyCanvas  = m_dirtyCanvas;
                    unlock();

                    //canvas size or tile size may change during painting tiles
                    if (dirtyCanvas) {
                        if (m_threadRendering)
                            QMetaObject::invokeMethod(this, "paint", Qt::QueuedConnection);
                        endPainting();
                        return;
                    } else if (dirtyTile) {
                        ccb->replay(tile->createPainter(smooth), oldState);
                        tile->drawFinished();
                        lock();
                        tile->markDirty(false);
                        unlock();
                    }

                    compositeTile(tile);
                }
                ccb->clear();
                endPainting();
                m_state = oldState;
                markDirtyTexture();
            }
        }
    }
}

QRect QQuickContext2DTexture::tiledRect(const QRectF& window, const QSize& tileSize)
{
    if (window.isEmpty())
        return QRect();

    const int tw = tileSize.width();
    const int th = tileSize.height();
    const int h1 = window.left() / tw;
    const int v1 = window.top() / th;

    const int htiles = ((window.right() - h1 * tw) + tw - 1)/tw;
    const int vtiles = ((window.bottom() - v1 * th) + th - 1)/th;

    return QRect(h1 * tw, v1 * th, htiles * tw, vtiles * th);
}

QRect QQuickContext2DTexture::createTiles(const QRect& window)
{
    QList<QQuickContext2DTile*> oldTiles = m_tiles;
    m_tiles.clear();

    if (window.isEmpty()) {
        m_dirtyCanvas = false;
        return QRect();
    }

    QRect r = tiledRect(window, m_tileSize);

    const int tw = m_tileSize.width();
    const int th = m_tileSize.height();
    const int h1 = window.left() / tw;
    const int v1 = window.top() / th;


    const int htiles = r.width() / tw;
    const int vtiles = r.height() / th;

    for (int yy = 0; yy < vtiles; ++yy) {
        for (int xx = 0; xx < htiles; ++xx) {
            int ht = xx + h1;
            int vt = yy + v1;

            QQuickContext2DTile* tile = 0;

            QPoint pos(ht * tw, vt * th);
            QRect rect(pos, m_tileSize);

            for (int i = 0; i < oldTiles.size(); i++) {
                if (oldTiles[i]->rect() == rect) {
                    tile = oldTiles.takeAt(i);
                    break;
                }
            }

            if (!tile)
                tile = createTile();

            tile->setRect(rect);
            m_tiles.append(tile);
        }
    }

    qDeleteAll(oldTiles);

    m_dirtyCanvas = false;
    return r;
}

void QQuickContext2DTexture::clearTiles()
{
    qDeleteAll(m_tiles);
    m_tiles.clear();
}

QQuickContext2DFBOTexture::QQuickContext2DFBOTexture()
    : QQuickContext2DTexture()
    , m_fbo(0)
    , m_multisampledFbo(0)
    , m_paint_device(0)
{
    m_threadRendering = false;
}

QQuickContext2DFBOTexture::~QQuickContext2DFBOTexture()
{
    delete m_fbo;
    delete m_multisampledFbo;
    delete m_paint_device;
}

bool QQuickContext2DFBOTexture::setCanvasSize(const QSize &size)
{
    QSize s = QSize(qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(size.width()))
                  , qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(size.height())));

    if (m_canvasSize != s) {
        m_canvasSize = s;
        m_dirtyCanvas = true;
        return true;
    }
    return false;
}

bool QQuickContext2DFBOTexture::setTileSize(const QSize &size)
{
    QSize s = QSize(qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(size.width()))
                  , qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(size.height())));
    if (m_tileSize != s) {
        m_tileSize = s;
        m_dirtyCanvas = true;
        return true;
    }
    return false;
}

bool QQuickContext2DFBOTexture::setCanvasWindow(const QRect& canvasWindow)
{
    QSize s = QSize(qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(canvasWindow.size().width()))
                  , qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(canvasWindow.size().height())));


    bool doChanged = false;
    if (m_fboSize != s) {
        m_fboSize = s;
        doChanged = true;
    }

    if (m_canvasWindow != canvasWindow)
        m_canvasWindow = canvasWindow;

    return doChanged;
}

void QQuickContext2DFBOTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, textureId());
    updateBindOptions();
}

QRectF QQuickContext2DFBOTexture::textureSubRect() const
{
    return QRectF(0
                , 0
                , qreal(m_canvasWindow.width()) / m_fboSize.width()
                , qreal(m_canvasWindow.height()) / m_fboSize.height());
}


int QQuickContext2DFBOTexture::textureId() const
{
    return m_fbo? m_fbo->texture() : 0;
}


bool QQuickContext2DFBOTexture::updateTexture()
{
    if (!m_context->buffer()->isEmpty()) {
        paint();
    }

    bool textureUpdated = m_dirtyTexture;

    m_dirtyTexture = false;

    if (m_doGrabImage) {
        grabImage();
        m_condition.wakeOne();
        m_doGrabImage = false;
    }
    return textureUpdated;
}

QQuickContext2DTile* QQuickContext2DFBOTexture::createTile() const
{
    return new QQuickContext2DFBOTile();
}

void QQuickContext2DFBOTexture::grabImage()
{
    if (m_fbo) {
        m_grabedImage = m_fbo->toImage();
    }
}

bool QQuickContext2DFBOTexture::doMultisampling() const
{
    static bool extensionsChecked = false;
    static bool multisamplingSupported = false;

    if (!extensionsChecked) {
        QList<QByteArray> extensions = QByteArray((const char *)glGetString(GL_EXTENSIONS)).split(' ');
        multisamplingSupported = extensions.contains("GL_EXT_framebuffer_multisample")
                && extensions.contains("GL_EXT_framebuffer_blit");
        extensionsChecked = true;
    }

    return multisamplingSupported  && m_smooth;
}

QImage QQuickContext2DFBOTexture::toImage(const QRectF& region)
{
#define QML_CONTEXT2D_WAIT_MAX 5000

    m_doGrabImage = true;
    if (m_item)
        m_item->update();

    QImage grabbed;
    m_mutex.lock();
    bool ok = m_condition.wait(&m_mutex, QML_CONTEXT2D_WAIT_MAX);

    if (!ok)
        grabbed = QImage();

    if (region.isValid())
        grabbed = m_grabedImage.copy(region.toRect());
    else
        grabbed = m_grabedImage;
    m_grabedImage = QImage();
    return grabbed;
}

void QQuickContext2DFBOTexture::compositeTile(QQuickContext2DTile* tile)
{
    QQuickContext2DFBOTile* t = static_cast<QQuickContext2DFBOTile*>(tile);
    QRect target = t->rect().intersect(m_canvasWindow);
    if (target.isValid()) {
        QRect source = target;

        source.moveTo(source.topLeft() - t->rect().topLeft());
        target.moveTo(target.topLeft() - m_canvasWindow.topLeft());

        QOpenGLFramebufferObject::blitFramebuffer(m_fbo, target, t->fbo(), source);
    }
}
QQuickCanvasItem::RenderTarget QQuickContext2DFBOTexture::renderTarget() const
{
    return QQuickCanvasItem::FramebufferObject;
}
QPaintDevice* QQuickContext2DFBOTexture::beginPainting()
{
    QQuickContext2DTexture::beginPainting();

    if (m_canvasWindow.size().isEmpty() && !m_threadRendering) {
        delete m_fbo;
        delete m_multisampledFbo;
        m_fbo = 0;
        m_multisampledFbo = 0;
        return 0;
    } else if (!m_fbo || m_fbo->size() != m_fboSize) {
        delete m_fbo;
        delete m_multisampledFbo;
        if (doMultisampling()) {
            {
                QOpenGLFramebufferObjectFormat format;
                format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
                format.setSamples(8);
                m_multisampledFbo = new QOpenGLFramebufferObject(m_fboSize, format);
            }
            {
                QOpenGLFramebufferObjectFormat format;
                format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
                m_fbo = new QOpenGLFramebufferObject(m_fboSize, format);
            }
        } else {
            QOpenGLFramebufferObjectFormat format;
            format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

            m_fbo = new QOpenGLFramebufferObject(m_fboSize, format);
        }
    }

    if (doMultisampling())
        m_multisampledFbo->bind();
    else
        m_fbo->bind();


    if (!m_paint_device) {
        QOpenGLPaintDevice *gl_device = new QOpenGLPaintDevice(m_fbo->size());
        gl_device->setPaintFlipped(true);
        m_paint_device = gl_device;
    }

    return m_paint_device;
}

void QQuickContext2DFBOTexture::endPainting()
{
    QQuickContext2DTexture::endPainting();
    if (m_multisampledFbo) {
        QOpenGLFramebufferObject::blitFramebuffer(m_fbo, m_multisampledFbo);
        m_multisampledFbo->release();
    } else if (m_fbo)
        m_fbo->release();
}
void qt_quit_context2d_render_thread()
{
    QThread* thread = globalCanvasThreadRenderInstance();

    if (thread->isRunning()) {
        thread->exit(0);
        thread->wait(1000);
    }
}

QQuickContext2DImageTexture::QQuickContext2DImageTexture(bool threadRendering)
    : QQuickContext2DTexture()
    , m_texture(new QSGPlainTexture())
{
    m_texture->setOwnsTexture(true);
    m_texture->setHasMipmaps(false);

    m_threadRendering = threadRendering;

    if (m_threadRendering) {
        QThread* thread = globalCanvasThreadRenderInstance();
        moveToThread(thread);

        if (!thread->isRunning()) {
            qAddPostRoutine(qt_quit_context2d_render_thread);
            thread->start();
        }
    }
}

QQuickContext2DImageTexture::~QQuickContext2DImageTexture()
{
    delete m_texture;
}

int QQuickContext2DImageTexture::textureId() const
{
    return m_texture->textureId();
}

void QQuickContext2DImageTexture::lock()
{
    if (m_threadRendering)
        m_mutex.lock();
}
void QQuickContext2DImageTexture::unlock()
{
    if (m_threadRendering)
        m_mutex.unlock();
}

void QQuickContext2DImageTexture::wait()
{
    if (m_threadRendering)
        m_waitCondition.wait(&m_mutex);
}

void QQuickContext2DImageTexture::wake()
{
    if (m_threadRendering)
        m_waitCondition.wakeOne();
}

bool QQuickContext2DImageTexture::supportDirectRendering() const
{
    return !m_threadRendering;
}

QQuickCanvasItem::RenderTarget QQuickContext2DImageTexture::renderTarget() const
{
    return QQuickCanvasItem::Image;
}

void QQuickContext2DImageTexture::bind()
{
    m_texture->bind();
}

bool QQuickContext2DImageTexture::updateTexture()
{
    lock();
    bool textureUpdated = m_dirtyTexture;
    if (m_dirtyTexture) {
        m_texture->setImage(m_image);
        m_dirtyTexture = false;
    }
    unlock();
    return textureUpdated;
}

QQuickContext2DTile* QQuickContext2DImageTexture::createTile() const
{
    return new QQuickContext2DImageTile();
}

void QQuickContext2DImageTexture::grabImage(const QRect& r)
{
    m_doGrabImage = true;
    paint();
    m_doGrabImage = false;
    m_grabedImage = m_image.copy(r);
}

QImage QQuickContext2DImageTexture::toImage(const QRectF& region)
{
    QRect r = region.isValid() ? region.toRect() : QRect(QPoint(0, 0), m_canvasWindow.size());
    if (threadRendering()) {
        wake();
        QMetaObject::invokeMethod(this, "grabImage", Qt::BlockingQueuedConnection, Q_ARG(QRect, r));
    } else {
        QMetaObject::invokeMethod(this, "grabImage", Qt::DirectConnection, Q_ARG(QRect, r));
    }
    QImage image = m_grabedImage;
    m_grabedImage = QImage();
    return image;
}

QPaintDevice* QQuickContext2DImageTexture::beginPainting()
{
     QQuickContext2DTexture::beginPainting();

    if (m_canvasWindow.size().isEmpty())
        return 0;

    lock();
    if (m_image.size() != m_canvasWindow.size()) {
        m_image = QImage(m_canvasWindow.size(), QImage::Format_ARGB32_Premultiplied);
        m_image.fill(0x00000000);
    }
    unlock();
    return &m_image;
}

void QQuickContext2DImageTexture::compositeTile(QQuickContext2DTile* tile)
{
    Q_ASSERT(!tile->dirty());
    QQuickContext2DImageTile* t = static_cast<QQuickContext2DImageTile*>(tile);
    QRect target = t->rect().intersect(m_canvasWindow);
    if (target.isValid()) {
        QRect source = target;
        source.moveTo(source.topLeft() - t->rect().topLeft());
        target.moveTo(target.topLeft() - m_canvasWindow.topLeft());

        lock();
        m_painter.begin(&m_image);
        m_painter.setCompositionMode(QPainter::CompositionMode_Source);
        m_painter.drawImage(target, t->image(), source);
        m_painter.end();
        unlock();
    }
}
