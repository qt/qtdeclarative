/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

QT_BEGIN_NAMESPACE

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

struct GLAcquireContext {
    GLAcquireContext(QOpenGLContext *c, QSurface *s):ctx(c) {
        if (ctx) {
            Q_ASSERT(s);
            if (!ctx->isValid())
                ctx->create();

            if (!ctx->isValid())
                qWarning() << "Unable to create GL context";
            else if (!ctx->makeCurrent(s))
                qWarning() << "Can't make current GL context";
        }
    }
    ~GLAcquireContext() {
        if (ctx)
            ctx->doneCurrent();
    }
    QOpenGLContext *ctx;
};

QQuickContext2DTexture::QQuickContext2DTexture()
    : m_context(0)
    , m_item(0)
    , m_dirtyCanvas(false)
    , m_canvasWindowChanged(false)
    , m_dirtyTexture(false)
    , m_smooth(true)
    , m_antialiasing(false)
    , m_tiledCanvas(false)
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
    m_dirtyTexture = true;
    updateTexture();
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

void QQuickContext2DTexture::setAntialiasing(bool antialiasing)
{
    m_antialiasing = antialiasing;
}

void QQuickContext2DTexture::setItem(QQuickCanvasItem* item)
{
    m_item = item;
    m_context = (QQuickContext2D*)item->rawContext(); // FIXME
    m_state = m_context->state;
}

bool QQuickContext2DTexture::setCanvasWindow(const QRect& r)
{
    if (m_canvasWindow != r) {
        m_canvasWindow = r;
        m_canvasWindowChanged = true;
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

void QQuickContext2DTexture::canvasChanged(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth, bool antialiasing)
{
    QSize ts = tileSize;
    if (ts.width() > canvasSize.width())
        ts.setWidth(canvasSize.width());

    if (ts.height() > canvasSize.height())
        ts.setHeight(canvasSize.height());

    setCanvasSize(canvasSize);
    setTileSize(ts);
    setCanvasWindow(canvasWindow);

    if (canvasSize == canvasWindow.size()) {
        m_tiledCanvas = false;
        m_dirtyCanvas = false;
    } else {
        m_tiledCanvas = true;
    }

    if (dirtyRect.isValid())
        setDirtyRect(dirtyRect);

    setSmooth(smooth);
    setAntialiasing(antialiasing);
}

void QQuickContext2DTexture::paintWithoutTiles(QQuickContext2DCommandBuffer *ccb)
{
    if (!ccb || ccb->isEmpty())
        return;

    QPaintDevice* device = beginPainting();
    if (!device) {
        endPainting();
        return;
    }

    QPainter p;
    p.begin(device);
    if (m_antialiasing)
        p.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing, true);
    else
        p.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing, false);

    if (m_smooth)
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    else
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    ccb->replay(&p, m_state);
    endPainting();
    markDirtyTexture();
}

bool QQuickContext2DTexture::canvasDestroyed()
{
    return m_item == 0;
}

void QQuickContext2DTexture::paint(QQuickContext2DCommandBuffer *ccb)
{
    if (canvasDestroyed()) {
        delete ccb;
        return;
    }

    GLAcquireContext currentContext(m_context->glContext(), m_context->surface());

    if (!m_tiledCanvas) {
        paintWithoutTiles(ccb);
        delete ccb;
        return;
    }

    QRect tiledRegion = createTiles(m_canvasWindow.intersected(QRect(QPoint(0, 0), m_canvasSize)));
    if (!tiledRegion.isEmpty()) {
        QRect dirtyRect;
        foreach (QQuickContext2DTile* tile, m_tiles) {
            if (tile->dirty()) {
                if (dirtyRect.isEmpty())
                    dirtyRect = tile->rect();
                else
                    dirtyRect |= tile->rect();
            }
        }

        if (beginPainting()) {
            QQuickContext2D::State oldState = m_state;
            foreach (QQuickContext2DTile* tile, m_tiles) {
                if (tile->dirty()) {
                    ccb->replay(tile->createPainter(m_smooth, m_antialiasing), oldState);
                    tile->drawFinished();
                    tile->markDirty(false);
                }
                compositeTile(tile);
            }
            endPainting();
            m_state = oldState;
            markDirtyTexture();
        }
    }
    delete ccb;
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

    QRect r = tiledRect(window, adjustedTileSize(m_tileSize));

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

QSize QQuickContext2DTexture::adjustedTileSize(const QSize &ts)
{
    return ts;
}

static inline QSize npotAdjustedSize(const QSize &size)
{
    static bool checked = false;
    static bool npotSupported = false;

    if (!checked) {
        npotSupported = QOpenGLContext::currentContext()->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
        checked = true;
    }

    if (npotSupported) {
        return QSize(qMax(QT_MINIMUM_FBO_SIZE, size.width()),
                     qMax(QT_MINIMUM_FBO_SIZE, size.height()));
    }

    return QSize(qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(size.width())),
                       qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(size.height())));
}

QQuickContext2DFBOTexture::QQuickContext2DFBOTexture()
    : QQuickContext2DTexture()
    , m_fbo(0)
    , m_multisampledFbo(0)
    , m_paint_device(0)
{
}

QQuickContext2DFBOTexture::~QQuickContext2DFBOTexture()
{
    if (m_multisampledFbo)
        m_multisampledFbo->release();
    else if (m_fbo)
        m_fbo->release();

    delete m_fbo;
    delete m_multisampledFbo;
    delete m_paint_device;
}

QSize QQuickContext2DFBOTexture::adjustedTileSize(const QSize &ts)
{
    return npotAdjustedSize(ts);
}

void QQuickContext2DFBOTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, textureId());
    updateBindOptions();
}

QRectF QQuickContext2DFBOTexture::normalizedTextureSubRect() const
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
    bool textureUpdated = m_dirtyTexture;
    m_dirtyTexture = false;
    return textureUpdated;
}

QQuickContext2DTile* QQuickContext2DFBOTexture::createTile() const
{
    return new QQuickContext2DFBOTile();
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

    return multisamplingSupported  && m_antialiasing;
}

void QQuickContext2DFBOTexture::grabImage(const QRectF& rf)
{
    Q_ASSERT(rf.isValid());

    if (!m_fbo) {
        m_context->setGrabbedImage(QImage());
        return;
    }

    QImage grabbed;
    {
        GLAcquireContext ctx(m_context->glContext(), m_context->surface());
        grabbed = m_fbo->toImage().mirrored().copy(rf.toRect());
    }

    m_context->setGrabbedImage(grabbed);
}

void QQuickContext2DFBOTexture::compositeTile(QQuickContext2DTile* tile)
{
    QQuickContext2DFBOTile* t = static_cast<QQuickContext2DFBOTile*>(tile);
    QRect target = t->rect().intersected(m_canvasWindow);
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

    if (m_canvasWindow.size().isEmpty()) {
        delete m_fbo;
        delete m_multisampledFbo;
        delete m_paint_device;
        m_fbo = 0;
        m_multisampledFbo = 0;
        m_paint_device = 0;
        return 0;
    } else if (!m_fbo || m_canvasWindowChanged) {
        delete m_fbo;
        delete m_multisampledFbo;
        delete m_paint_device;
        m_paint_device = 0;

        m_fboSize = npotAdjustedSize(m_canvasWindow.size());
        m_canvasWindowChanged = false;

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
        gl_device->setSize(m_fbo->size());
        m_paint_device = gl_device;
    }

    return m_paint_device;
}

void QQuickContext2DFBOTexture::endPainting()
{
    QQuickContext2DTexture::endPainting();
    if (m_multisampledFbo)
        QOpenGLFramebufferObject::blitFramebuffer(m_fbo, m_multisampledFbo);
}

QQuickContext2DImageTexture::QQuickContext2DImageTexture()
    : QQuickContext2DTexture()
    , m_texture(0)
{
}

QQuickContext2DImageTexture::~QQuickContext2DImageTexture()
{
    if (m_texture && m_texture->thread() != QThread::currentThread())
        m_texture->deleteLater();
    else
        delete m_texture;
}

int QQuickContext2DImageTexture::textureId() const
{
    return imageTexture()->textureId();
}

QQuickCanvasItem::RenderTarget QQuickContext2DImageTexture::renderTarget() const
{
    return QQuickCanvasItem::Image;
}

void QQuickContext2DImageTexture::bind()
{
    imageTexture()->bind();
    updateBindOptions();
}

bool QQuickContext2DImageTexture::updateTexture()
{
    bool textureUpdated = m_dirtyTexture;
    if (m_dirtyTexture) {
        imageTexture()->setImage(m_image);
        m_dirtyTexture = false;
    }
    return textureUpdated;
}

QQuickContext2DTile* QQuickContext2DImageTexture::createTile() const
{
    return new QQuickContext2DImageTile();
}

void QQuickContext2DImageTexture::grabImage(const QRectF& rf)
{
    Q_ASSERT(rf.isValid());
    Q_ASSERT(m_context);
    QImage grabbed = m_image.copy(rf.toRect());
    m_context->setGrabbedImage(grabbed);
}

QSGPlainTexture *QQuickContext2DImageTexture::imageTexture() const
{
    if (!m_texture) {
        QQuickContext2DImageTexture *that = const_cast<QQuickContext2DImageTexture *>(this);
        that->m_texture = new QSGPlainTexture;
        that->m_texture->setOwnsTexture(true);
        that->m_texture->setHasMipmaps(false);
    }
    return m_texture;
}

QPaintDevice* QQuickContext2DImageTexture::beginPainting()
{
    QQuickContext2DTexture::beginPainting();

    if (m_canvasWindow.size().isEmpty())
        return 0;

    if (m_canvasWindowChanged) {
        m_image = QImage(m_canvasWindow.size(), QImage::Format_ARGB32_Premultiplied);
        m_image.fill(0x00000000);
        m_canvasWindowChanged = false;
    }

    return &m_image;
}

void QQuickContext2DImageTexture::compositeTile(QQuickContext2DTile* tile)
{
    Q_ASSERT(!tile->dirty());
    QQuickContext2DImageTile* t = static_cast<QQuickContext2DImageTile*>(tile);
    QRect target = t->rect().intersected(m_canvasWindow);
    if (target.isValid()) {
        QRect source = target;
        source.moveTo(source.topLeft() - t->rect().topLeft());
        target.moveTo(target.topLeft() - m_canvasWindow.topLeft());

        m_painter.begin(&m_image);
        m_painter.setCompositionMode(QPainter::CompositionMode_Source);
        m_painter.drawImage(target, t->image(), source);
        m_painter.end();
    }
}

QT_END_NAMESPACE
