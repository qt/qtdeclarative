// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcontext2dtexture_p.h"
#include "qquickcontext2dtile_p.h"
#include "qquickcanvasitem_p.h"
#include <private/qquickitem_p.h>
#include <QtQuick/private/qsgplaintexture_p.h>
#include "qquickcontext2dcommandbuffer_p.h"
#include <QtCore/QThread>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcCanvas, "qt.quick.canvas")

QQuickContext2DTexture::QQuickContext2DTexture()
    : m_context(nullptr)
    , m_surface(nullptr)
    , m_item(nullptr)
    , m_canvasDevicePixelRatio(1)
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

void QQuickContext2DTexture::markDirtyTexture()
{
    if (m_onCustomThread)
        m_mutex.lock();
    m_dirtyTexture = true;
    emit textureChanged();
    if (m_onCustomThread)
        m_mutex.unlock();
}

bool QQuickContext2DTexture::setCanvasSize(const QSize &size)
{
    if (m_canvasSize != size) {
        m_canvasSize = size;
        return true;
    }
    return false;
}

bool QQuickContext2DTexture::setTileSize(const QSize &size)
{
    if (m_tileSize != size) {
        m_tileSize = size;
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
    if (m_item) {
        m_context = (QQuickContext2D*) item->rawContext(); // FIXME
        m_state = m_context->state;
    } else {
        m_context = nullptr;
    }
}

bool QQuickContext2DTexture::setCanvasWindow(const QRect& r)
{
    bool ok = false;
    static qreal overriddenDevicePixelRatio =
        !qEnvironmentVariableIsEmpty("QT_CANVAS_OVERRIDE_DEVICEPIXELRATIO") ?
        qgetenv("QT_CANVAS_OVERRIDE_DEVICEPIXELRATIO").toFloat(&ok) : 0.0;
    qreal canvasDevicePixelRatio = overriddenDevicePixelRatio;
    if (overriddenDevicePixelRatio == 0.0) {
        canvasDevicePixelRatio = (m_item && m_item->window()) ?
            m_item->window()->effectiveDevicePixelRatio() : qApp->devicePixelRatio();
    }
    if (!qFuzzyCompare(m_canvasDevicePixelRatio, canvasDevicePixelRatio)) {
        qCDebug(lcCanvas, "%s device pixel ratio %.1lf -> %.1lf",
                (m_item->objectName().isEmpty() ? "Canvas" : qPrintable(m_item->objectName())),
                m_canvasDevicePixelRatio, canvasDevicePixelRatio);
        m_canvasDevicePixelRatio = canvasDevicePixelRatio;
        m_canvasWindowChanged = true;
    }

    if (m_canvasWindow != r) {
        m_canvasWindow = r;
        m_canvasWindowChanged = true;
    }

    return m_canvasWindowChanged;
}

bool QQuickContext2DTexture::setDirtyRect(const QRect &r)
{
    bool doDirty = false;
    if (m_tiledCanvas) {
        for (QQuickContext2DTile* t : std::as_const(m_tiles)) {
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
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, m_antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    ccb->replay(&p, m_state, scaleFactor());
    endPainting();
    markDirtyTexture();
}

bool QQuickContext2DTexture::canvasDestroyed()
{
    return m_item == nullptr;
}

void QQuickContext2DTexture::paint(QQuickContext2DCommandBuffer *ccb)
{
    QQuickContext2D::mutex.lock();
    if (canvasDestroyed()) {
        delete ccb;
        QQuickContext2D::mutex.unlock();
        return;
    }
    QQuickContext2D::mutex.unlock();
    if (!m_tiledCanvas) {
        paintWithoutTiles(ccb);
        delete ccb;
        return;
    }

    QRect tiledRegion = createTiles(m_canvasWindow.intersected(QRect(QPoint(0, 0), m_canvasSize)));
    if (!tiledRegion.isEmpty()) {
        QRect dirtyRect;
        for (QQuickContext2DTile* tile : std::as_const(m_tiles)) {
            if (tile->dirty()) {
                if (dirtyRect.isEmpty())
                    dirtyRect = tile->rect();
                else
                    dirtyRect |= tile->rect();
            }
        }

        if (beginPainting()) {
            QQuickContext2D::State oldState = m_state;
            for (QQuickContext2DTile* tile : std::as_const(m_tiles)) {
                if (tile->dirty()) {
                    ccb->replay(tile->createPainter(m_smooth, m_antialiasing), oldState, scaleFactor());
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

            QQuickContext2DTile* tile = nullptr;

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

bool QQuickContext2DTexture::event(QEvent *e)
{
    if ((int) e->type() == QEvent::User + 1) {
        PaintEvent *pe = static_cast<PaintEvent *>(e);
        paint(pe->buffer);
        return true;
    } else if ((int) e->type() == QEvent::User + 2) {
        CanvasChangeEvent *ce = static_cast<CanvasChangeEvent *>(e);
        canvasChanged(ce->canvasSize, ce->tileSize, ce->canvasWindow, ce->dirtyRect, ce->smooth, ce->antialiasing);
        return true;
    }
    return QObject::event(e);
}

QQuickContext2DImageTexture::QQuickContext2DImageTexture()
    : QQuickContext2DTexture()
{
}

QQuickContext2DImageTexture::~QQuickContext2DImageTexture()
{
}

QQuickCanvasItem::RenderTarget QQuickContext2DImageTexture::renderTarget() const
{
    return QQuickCanvasItem::Image;
}

QQuickContext2DTile* QQuickContext2DImageTexture::createTile() const
{
    return new QQuickContext2DImageTile();
}

void QQuickContext2DImageTexture::grabImage(const QRectF& rf)
{
    Q_ASSERT(rf.isValid());
    QQuickContext2D::mutex.lock();
    if (m_context) {
        QImage grabbed = m_displayImage.copy(rf.toRect());
        m_context->setGrabbedImage(grabbed);
    }
    QQuickContext2D::mutex.unlock();
}

QSGTexture *QQuickContext2DImageTexture::textureForNextFrame(QSGTexture *last, QQuickWindow *window)
{
    if (m_onCustomThread)
        m_mutex.lock();

    delete last;

    QSGTexture *texture = window->createTextureFromImage(m_displayImage, QQuickWindow::TextureCanUseAtlas);
    m_dirtyTexture = false;

    if (m_onCustomThread)
        m_mutex.unlock();

    return texture;
}

QPaintDevice* QQuickContext2DImageTexture::beginPainting()
{
    QQuickContext2DTexture::beginPainting();

    if (m_canvasWindow.size().isEmpty())
        return nullptr;


    if (m_canvasWindowChanged) {
        m_image = QImage(m_canvasWindow.size() * m_canvasDevicePixelRatio, QImage::Format_ARGB32_Premultiplied);
        m_image.setDevicePixelRatio(m_canvasDevicePixelRatio);
        m_image.fill(0x00000000);
        m_canvasWindowChanged = false;
        qCDebug(lcCanvas, "%s size %.1lf x %.1lf painting with size %d x %d DPR %.1lf",
                (m_item->objectName().isEmpty() ? "Canvas" : qPrintable(m_item->objectName())),
                m_item->width(), m_item->height(), m_image.size().width(), m_image.size().height(), m_canvasDevicePixelRatio);
    }

    return &m_image;
}

void QQuickContext2DImageTexture::endPainting()
{
    QQuickContext2DTexture::endPainting();
    if (m_onCustomThread)
        m_mutex.lock();
    m_displayImage = m_image;
    if (m_onCustomThread)
        m_mutex.unlock();
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

#include "moc_qquickcontext2dtexture_p.cpp"
