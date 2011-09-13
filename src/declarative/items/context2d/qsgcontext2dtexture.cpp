#include "qsgcontext2dtexture_p.h"
#include "qsgcontext2dtile_p.h"
#include "qsgcanvasitem_p.h"
#include "qsgitem_p.h"
#include "private/qsgtexture_p.h"
#include "qsgcontext2dcommandbuffer_p.h"
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


QSGContext2DTexture::QSGContext2DTexture()
    : QSGDynamicTexture()
    , m_context(0)
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

QSGContext2DTexture::~QSGContext2DTexture()
{
   clearTiles();
}

QSize QSGContext2DTexture::textureSize() const
{
    return m_canvasWindow.size();
}

void QSGContext2DTexture::markDirtyTexture()
{
    lock();
    m_dirtyTexture = true;
    unlock();
    emit textureChanged();
}

bool QSGContext2DTexture::setCanvasSize(const QSize &size)
{
    if (m_canvasSize != size) {
        m_canvasSize = size;
        m_dirtyCanvas = true;
        return true;
    }
    return false;
}

bool QSGContext2DTexture::setTileSize(const QSize &size)
{
    if (m_tileSize != size) {
        m_tileSize = size;
        m_dirtyCanvas = true;
        return true;
    }
    return false;
}

void QSGContext2DTexture::setSmooth(bool smooth)
{
    m_smooth = smooth;
}

void QSGContext2DTexture::setItem(QSGCanvasItem* item)
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
        connect(this, SIGNAL(textureChanged()), m_item, SIGNAL(painted()), Qt::QueuedConnection);
        canvasChanged(item->canvasSize().toSize()
                    , item->tileSize()
                    , item->canvasWindow().toAlignedRect()
                    , item->canvasWindow().toAlignedRect()
                    , item->smooth());
    }
}

bool QSGContext2DTexture::setCanvasWindow(const QRect& r)
{
    if (m_canvasWindow != r) {
        m_canvasWindow = r;
    }
}

bool QSGContext2DTexture::setDirtyRect(const QRect &r)
{
    bool doDirty = false;
    foreach (QSGContext2DTile* t, m_tiles) {
        bool dirty = t->rect().intersected(r).isValid();
        t->markDirty(dirty);
        if (dirty)
            doDirty = true;
    }
    return doDirty;
}

void QSGContext2DTexture::canvasChanged(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth)
{
    lock();

    QSize ts = tileSize;
    if (ts.width() > canvasSize.width())
        ts.setWidth(canvasSize.width());

    if (ts.height() > canvasSize.height())
        ts.setHeight(canvasSize.height());

    bool canvasChanged = setCanvasSize(canvasSize);
    bool tileChanged = setTileSize(ts);

    bool doDirty = false;
    if (canvasSize == canvasWindow.size()) {
        m_tiledCanvas = false;
        m_dirtyCanvas = false;
    } else {
        m_tiledCanvas = true;
        if (dirtyRect.isValid())
            doDirty = setDirtyRect(dirtyRect);
    }

    bool windowChanged = setCanvasWindow(canvasWindow);

    if (windowChanged || doDirty) {
        if (m_threadRendering)
            QMetaObject::invokeMethod(this, "paint", Qt::QueuedConnection);
        else if (supportDirectRendering())
            QMetaObject::invokeMethod(this, "paint", Qt::DirectConnection);
    }

    setSmooth(smooth);
    unlock();
}

void QSGContext2DTexture::paintWithoutTiles()
{
    QSGContext2DCommandBuffer* ccb = m_context->buffer();

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
    m_state = ccb->replay(&p, m_state);

    ccb->clear();
    markDirtyTexture();
    endPainting();
}

bool QSGContext2DTexture::canvasDestroyed()
{
    bool noCanvas = false;
    lock();
    noCanvas = m_item == 0;
    unlock();
    return noCanvas;
}

void QSGContext2DTexture::paint()
{
    if (canvasDestroyed())
        return;

    if (!m_tiledCanvas) {
        paintWithoutTiles();
    } else {
        QSGContext2D::State oldState = m_state;
        QSGContext2DCommandBuffer* ccb = m_context->buffer();

        lock();
        QRect tiledRegion = createTiles(m_canvasWindow.intersected(QRect(QPoint(0, 0), m_canvasSize)));
        unlock();

        if (!tiledRegion.isEmpty()) {
            if (m_threadRendering && !m_doGrabImage) {
                QRect dirtyRect;

                lock();
                foreach (QSGContext2DTile* tile, m_tiles) {
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
                foreach (QSGContext2DTile* tile, m_tiles) {
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
                        m_state = ccb->replay(tile->createPainter(smooth), oldState);

                        lock();
                        tile->markDirty(false);
                        unlock();
                    }

                    compositeTile(tile);
                }
                ccb->clear();
                endPainting();
                markDirtyTexture();
            }
        }
    }
}

QRect QSGContext2DTexture::tiledRect(const QRectF& window, const QSize& tileSize)
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

QRect QSGContext2DTexture::createTiles(const QRect& window)
{
    QList<QSGContext2DTile*> oldTiles = m_tiles;
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

            QSGContext2DTile* tile = 0;

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

void QSGContext2DTexture::clearTiles()
{
    qDeleteAll(m_tiles);
    m_tiles.clear();
}

QSGContext2DFBOTexture::QSGContext2DFBOTexture()
    : QSGContext2DTexture()
    , m_fbo(0)
    , m_paint_device(0)
{
    m_threadRendering = false;
}

bool QSGContext2DFBOTexture::setCanvasSize(const QSize &size)
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

bool QSGContext2DFBOTexture::setTileSize(const QSize &size)
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

bool QSGContext2DFBOTexture::setCanvasWindow(const QRect& canvasWindow)
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

void QSGContext2DFBOTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, textureId());
    updateBindOptions();
}

QRectF QSGContext2DFBOTexture::textureSubRect() const
{
    return QRectF(0
                , 1
                , qreal(m_canvasWindow.width()) / m_fboSize.width()
                , qreal(-m_canvasWindow.height()) / m_fboSize.height());
}


int QSGContext2DFBOTexture::textureId() const
{
    return m_fbo? m_fbo->texture() : 0;
}


bool QSGContext2DFBOTexture::updateTexture()
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

QSGContext2DTile* QSGContext2DFBOTexture::createTile() const
{
    return new QSGContext2DFBOTile();
}

void QSGContext2DFBOTexture::grabImage()
{
    if (m_fbo) {
        m_grabedImage = m_fbo->toImage();
    }
}

QImage QSGContext2DFBOTexture::toImage(const QRectF& region)
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

void QSGContext2DFBOTexture::compositeTile(QSGContext2DTile* tile)
{
    QSGContext2DFBOTile* t = static_cast<QSGContext2DFBOTile*>(tile);
    QRect target = t->rect().intersect(m_canvasWindow);
    if (target.isValid()) {
        QRect source = target;

        source.moveTo(source.topLeft() - t->rect().topLeft());
        target.moveTo(target.topLeft() - m_canvasWindow.topLeft());

        QOpenGLFramebufferObject::blitFramebuffer(m_fbo, target, t->fbo(), source);
    }
}
QSGCanvasItem::RenderTarget QSGContext2DFBOTexture::renderTarget() const
{
    return QSGCanvasItem::FramebufferObject;
}
QPaintDevice* QSGContext2DFBOTexture::beginPainting()
{
    QSGContext2DTexture::beginPainting();

    if (m_canvasWindow.size().isEmpty() && !m_threadRendering) {
        delete m_fbo;
        m_fbo = 0;
    } else if (!m_fbo || m_fbo->size() != m_fboSize) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setInternalTextureFormat(GL_RGBA);
        format.setMipmap(false);
        format.setTextureTarget(GL_TEXTURE_2D);
        delete m_fbo;
        glDisable(GL_DEPTH_TEST);
        glDepthMask(false);

        m_fbo = new QOpenGLFramebufferObject(m_fboSize, format);
        glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
        updateBindOptions(false);
    }

    m_fbo->bind();

    if (!m_paint_device)
        m_paint_device = new QOpenGLPaintDevice(m_fbo->size());

    return m_paint_device;
}

void qt_quit_context2d_render_thread()
{
    QThread* thread = globalCanvasThreadRenderInstance();
    thread->quit();
    thread->wait();
}

QSGContext2DImageTexture::QSGContext2DImageTexture(bool threadRendering)
    : QSGContext2DTexture()
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

QSGContext2DImageTexture::~QSGContext2DImageTexture()
{
    m_texture->deleteLater();
}

int QSGContext2DImageTexture::textureId() const
{
    return m_texture->textureId();
}

void QSGContext2DImageTexture::lock()
{
    if (m_threadRendering)
        m_mutex.lock();
}
void QSGContext2DImageTexture::unlock()
{
    if (m_threadRendering)
        m_mutex.unlock();
}

void QSGContext2DImageTexture::wait()
{
    if (m_threadRendering)
        m_waitCondition.wait(&m_mutex);
}

void QSGContext2DImageTexture::wake()
{
    if (m_threadRendering)
        m_waitCondition.wakeOne();
}

bool QSGContext2DImageTexture::supportDirectRendering() const
{
    return !m_threadRendering;
}

QSGCanvasItem::RenderTarget QSGContext2DImageTexture::renderTarget() const
{
    return QSGCanvasItem::Image;
}

void QSGContext2DImageTexture::bind()
{
    m_texture->bind();
}

bool QSGContext2DImageTexture::updateTexture()
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

QSGContext2DTile* QSGContext2DImageTexture::createTile() const
{
    return new QSGContext2DImageTile();
}

void QSGContext2DImageTexture::grabImage(const QRect& r)
{
    m_doGrabImage = true;
    paint();
    m_doGrabImage = false;
    m_grabedImage = m_image.copy(r);
}

QImage QSGContext2DImageTexture::toImage(const QRectF& region)
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

QPaintDevice* QSGContext2DImageTexture::beginPainting()
{
     QSGContext2DTexture::beginPainting();

    if (m_canvasWindow.size().isEmpty())
        return 0;

    lock();
    if (m_image.size() != m_canvasWindow.size()) {
        m_image = QImage(m_canvasWindow.size(), QImage::Format_ARGB32_Premultiplied);
        m_image.fill(Qt::transparent);
    }
    unlock();
    return &m_image;
}

void QSGContext2DImageTexture::compositeTile(QSGContext2DTile* tile)
{
    Q_ASSERT(!tile->dirty());
    QSGContext2DImageTile* t = static_cast<QSGContext2DImageTile*>(tile);
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
