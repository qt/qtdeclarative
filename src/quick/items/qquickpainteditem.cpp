// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpainteditem.h"
#include <private/qquickpainteditem_p.h>

#include <QtQuick/private/qsgdefaultpainternode_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <qsgtextureprovider.h>
#include <rhi/qrhi.h>

#include <qmath.h>

QT_BEGIN_NAMESPACE

class QQuickPaintedItemTextureProvider : public QSGTextureProvider
{
public:
    QSGPainterNode *node;
    QSGTexture *texture() const override { return node ? node->texture() : nullptr; }
    void fireTextureChanged() { emit textureChanged(); }
};

/*!
    \class QQuickPaintedItem
    \brief The QQuickPaintedItem class provides a way to use the QPainter API in the
    QML Scene Graph.

    \inmodule QtQuick

    The QQuickPaintedItem makes it possible to use the QPainter API with the QML
    Scene Graph. It sets up a textured rectangle in the Scene Graph and uses a
    QPainter to paint onto the texture. The render target in Qt 6 is always a
    QImage.  When the render target is a QImage, QPainter first renders into the
    image then the content is uploaded to the texture.  Call update() to trigger
    a repaint.

    To enable QPainter to do anti-aliased rendering, use setAntialiasing().

    To write your own painted item, you first create a subclass of
    QQuickPaintedItem, and then start by implementing its only pure virtual
    public function: paint(), which implements the actual painting. The
    painting will be inside the rectangle spanning from 0,0 to
    width(),height().

    \note It important to understand the performance implications such items
    can incur. See QQuickPaintedItem::RenderTarget and
    QQuickPaintedItem::renderTarget.

    \sa {Scene Graph - Painted Item}, {Writing QML Extensions with C++}
*/

/*!
    \enum QQuickPaintedItem::RenderTarget

    This enum describes QQuickPaintedItem's render targets. The render target is the
    surface QPainter paints onto before the item is rendered on screen.

    \value Image The default; QPainter paints into a QImage using the raster paint engine.
    The image's content needs to be uploaded to graphics memory afterward, this operation
    can potentially be slow if the item is large. This render target allows high quality
    anti-aliasing and fast item resizing.

    \value FramebufferObject As of Qt 6.0, this value is ignored.

    \value InvertedYFramebufferObject As of Qt 6.0, this value is ignored.

    \sa setRenderTarget()
*/

/*!
    \enum QQuickPaintedItem::PerformanceHint

    This enum describes flags that you can enable to improve rendering
    performance in QQuickPaintedItem. By default, none of these flags are set.

    \value FastFBOResizing As of Qt 6.0, this value is ignored.
*/

/*!
    \internal
*/
QQuickPaintedItemPrivate::QQuickPaintedItemPrivate()
    : QQuickItemPrivate()
    , contentsScale(1.0)
    , fillColor(Qt::transparent)
    , renderTarget(QQuickPaintedItem::Image)
    , opaquePainting(false)
    , antialiasing(false)
    , mipmap(false)
    , textureProvider(nullptr)
    , node(nullptr)
{
}

/*!
    Constructs a QQuickPaintedItem with the given \a parent item.
 */
QQuickPaintedItem::QQuickPaintedItem(QQuickItem *parent)
    : QQuickItem(*(new QQuickPaintedItemPrivate), parent)
{
    setFlag(ItemHasContents);
}

/*!
    \internal
*/
QQuickPaintedItem::QQuickPaintedItem(QQuickPaintedItemPrivate &dd, QQuickItem *parent)
    : QQuickItem(dd, parent)
{
    setFlag(ItemHasContents);
}

/*!
    Destroys the QQuickPaintedItem.
*/
QQuickPaintedItem::~QQuickPaintedItem()
{
    Q_D(QQuickPaintedItem);
    if (d->textureProvider)
        QQuickWindowQObjectCleanupJob::schedule(window(), d->textureProvider);
}

/*!
    Schedules a redraw of the area covered by \a rect in this item. You can call this function
    whenever your item needs to be redrawn, such as if it changes appearance or size.

    This function does not cause an immediate paint; instead it schedules a paint request that
    is processed by the QML Scene Graph when the next frame is rendered. The item will only be
    redrawn if it is visible.

    \sa paint()
*/
void QQuickPaintedItem::update(const QRect &rect)
{
    Q_D(QQuickPaintedItem);

    if (rect.isNull() && !d->dirtyRect.isNull())
        d->dirtyRect = contentsBoundingRect().toAlignedRect();
    else
        d->dirtyRect |= (contentsBoundingRect() & rect).toAlignedRect();
    QQuickItem::update();
}

/*!
    Returns true if this item is opaque; otherwise, false is returned.

    By default, painted items are not opaque.

    \sa setOpaquePainting()
*/
bool QQuickPaintedItem::opaquePainting() const
{
    Q_D(const QQuickPaintedItem);
    return d->opaquePainting;
}

/*!
    If \a opaque is true, the item is opaque; otherwise, it is considered as translucent.

    Opaque items are not blended with the rest of the scene, you should set this to true
    if the content of the item is opaque to speed up rendering.

    By default, painted items are not opaque.

    \sa opaquePainting()
*/
void QQuickPaintedItem::setOpaquePainting(bool opaque)
{
    Q_D(QQuickPaintedItem);

    if (d->opaquePainting == opaque)
        return;

    d->opaquePainting = opaque;
    QQuickItem::update();
}

/*!
    Returns true if antialiased painting is enabled; otherwise, false is returned.

    By default, antialiasing is not enabled.

    \sa setAntialiasing()
*/
bool QQuickPaintedItem::antialiasing() const
{
    Q_D(const QQuickPaintedItem);
    return d->antialiasing;
}

/*!
    If \a enable is true, antialiased painting is enabled.

    By default, antialiasing is not enabled.

    \sa antialiasing()
*/
void QQuickPaintedItem::setAntialiasing(bool enable)
{
    Q_D(QQuickPaintedItem);

    if (d->antialiasing == enable)
        return;

    d->antialiasing = enable;
    update();
}

/*!
    Returns true if mipmaps are enabled; otherwise, false is returned.

    By default, mipmapping is not enabled.

    \sa setMipmap()
*/
bool QQuickPaintedItem::mipmap() const
{
    Q_D(const QQuickPaintedItem);
    return d->mipmap;
}

/*!
    If \a enable is true, mipmapping is enabled on the associated texture.

    Mipmapping increases rendering speed and reduces aliasing artifacts when the item is
    scaled down.

    By default, mipmapping is not enabled.

    \sa mipmap()
*/
void QQuickPaintedItem::setMipmap(bool enable)
{
    Q_D(QQuickPaintedItem);

    if (d->mipmap == enable)
        return;

    d->mipmap = enable;
    update();
}

/*!
    Returns the performance hints.

    By default, no performance hint is enabled.

    \sa setPerformanceHint(), setPerformanceHints()
*/
QQuickPaintedItem::PerformanceHints QQuickPaintedItem::performanceHints() const
{
    Q_D(const QQuickPaintedItem);
    return d->performanceHints;
}

/*!
    Sets the given performance \a hint on the item if \a enabled is true;
    otherwise clears the performance hint.

    By default, no performance hint is enabled/

    \sa setPerformanceHints(), performanceHints()
*/
void QQuickPaintedItem::setPerformanceHint(QQuickPaintedItem::PerformanceHint hint, bool enabled)
{
    Q_D(QQuickPaintedItem);
    PerformanceHints oldHints = d->performanceHints;
    if (enabled)
        d->performanceHints |= hint;
    else
        d->performanceHints &= ~hint;
    if (oldHints != d->performanceHints)
       update();
}

/*!
    Sets the performance hints to \a hints

    By default, no performance hint is enabled/

    \sa setPerformanceHint(), performanceHints()
*/
void QQuickPaintedItem::setPerformanceHints(QQuickPaintedItem::PerformanceHints hints)
{
    Q_D(QQuickPaintedItem);
    if (d->performanceHints == hints)
        return;
    d->performanceHints = hints;
    update();
}

QSize QQuickPaintedItem::textureSize() const
{
    Q_D(const QQuickPaintedItem);
    return d->textureSize;
}

/*!
    \property QQuickPaintedItem::textureSize

    \brief Defines the size of the texture.

    Changing the texture's size does not affect the coordinate system used in
    paint(). A scale factor is instead applied so painting should still happen
    inside 0,0 to width(),height().

    By default, the texture size will have the same size as this item.

    \note If the item is on a window with a device pixel ratio different from
    1, this scale factor will be implicitly applied to the texture size.

 */
void QQuickPaintedItem::setTextureSize(const QSize &size)
{
    Q_D(QQuickPaintedItem);
    if (d->textureSize == size)
        return;
    d->textureSize = size;
    emit textureSizeChanged();
}

/*!
    \deprecated

    This function is provided for compatibility, use size in combination
    with textureSize to decide the size of what you are drawing.

    \sa width(), height(), textureSize()
*/
QRectF QQuickPaintedItem::contentsBoundingRect() const
{
    Q_D(const QQuickPaintedItem);

    qreal w = d->width;
    QSizeF sz = d->contentsSize * d->contentsScale;
    if (w < sz.width())
        w = sz.width();
    qreal h = d->height;
    if (h < sz.height())
        h = sz.height();

    return QRectF(0, 0, w, h);
}

/*!
    \property QQuickPaintedItem::contentsSize
    \brief Obsolete method for setting the contents size.
    \deprecated

    This function is provided for compatibility, use size in combination
    with textureSize to decide the size of what you are drawing.

    \sa width(), height(), textureSize()
*/
QSize QQuickPaintedItem::contentsSize() const
{
    Q_D(const QQuickPaintedItem);
    return d->contentsSize;
}

void QQuickPaintedItem::setContentsSize(const QSize &size)
{
    Q_D(QQuickPaintedItem);

    if (d->contentsSize == size)
        return;

    d->contentsSize = size;
    update();

    emit contentsSizeChanged();
}

/*!
    \deprecated
    This convenience function is equivalent to calling setContentsSize(QSize()).
*/
void QQuickPaintedItem::resetContentsSize()
{
    setContentsSize(QSize());
}

/*!
    \property QQuickPaintedItem::contentsScale
    \brief Obsolete method for scaling the contents.
    \deprecated

    This function is provided for compatibility, use size() in combination
    with textureSize() to decide the size of what you are drawing.

    \sa width(), height(), textureSize()
*/
qreal QQuickPaintedItem::contentsScale() const
{
    Q_D(const QQuickPaintedItem);
    return d->contentsScale;
}

void QQuickPaintedItem::setContentsScale(qreal scale)
{
    Q_D(QQuickPaintedItem);

    if (d->contentsScale == scale)
        return;

    d->contentsScale = scale;
    update();

    emit contentsScaleChanged();
}

/*!
    \property QQuickPaintedItem::fillColor
    \brief The item's background fill color.

    By default, the fill color is set to Qt::transparent.

    Set the fill color to an invalid color (e.g. QColor()) to disable background
    filling. This may improve performance, and is safe to do if the paint() function
    draws to all pixels on each frame.
*/
QColor QQuickPaintedItem::fillColor() const
{
    Q_D(const QQuickPaintedItem);
    return d->fillColor;
}

void QQuickPaintedItem::setFillColor(const QColor &c)
{
    Q_D(QQuickPaintedItem);

    if (d->fillColor == c)
        return;

    d->fillColor = c;
    update();

    emit fillColorChanged();
}

/*!
    \property QQuickPaintedItem::renderTarget
    \brief The item's render target.

    This property defines which render target the QPainter renders into, it can be either
    QQuickPaintedItem::Image, QQuickPaintedItem::FramebufferObject or QQuickPaintedItem::InvertedYFramebufferObject.

    Each has certain benefits, typically performance versus quality. Using a framebuffer
    object avoids a costly upload of the image contents to the texture in graphics memory,
    while using an image enables high quality anti-aliasing.

    \warning Resizing a framebuffer object is a costly operation, avoid using
    the QQuickPaintedItem::FramebufferObject render target if the item gets resized often.

    By default, the render target is QQuickPaintedItem::Image.
*/
QQuickPaintedItem::RenderTarget QQuickPaintedItem::renderTarget() const
{
    Q_D(const QQuickPaintedItem);
    return d->renderTarget;
}

void QQuickPaintedItem::setRenderTarget(RenderTarget target)
{
    Q_D(QQuickPaintedItem);

    if (d->renderTarget == target)
        return;

    d->renderTarget = target;
    update();

    emit renderTargetChanged();
}

/*!
    \fn virtual void QQuickPaintedItem::paint(QPainter *painter) = 0

    This function, which is usually called by the QML Scene Graph, paints the
    contents of an item in local coordinates.

    The underlying texture will have a size defined by textureSize when set,
    or the item's size, multiplied by the window's device pixel ratio.

    The function is called after the item has been filled with the fillColor.

    Reimplement this function in a QQuickPaintedItem subclass to provide the
    item's painting implementation, using \a painter.

    \note The QML Scene Graph uses two separate threads, the main thread does
    things such as processing events or updating animations while a second
    thread does the actual issuing of graphics resource updates and the
    recording of draw calls.  As a consequence, paint() is not called from the
    main GUI thread but from the GL enabled renderer thread. At the moment
    paint() is called, the GUI thread is blocked and this is therefore
    thread-safe.

    \warning Extreme caution must be used when creating QObjects, emitting signals, starting
    timers and similar inside this function as these will have affinity to the rendering thread.

    \sa width(), height(), textureSize
*/

/*!
    \reimp
*/
QSGNode *QQuickPaintedItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickPaintedItem);

    if (width() <= 0 || height() <= 0) {
        delete oldNode;
        if (d->textureProvider) {
            d->textureProvider->node = nullptr;
            d->textureProvider->fireTextureChanged();
        }
        return nullptr;
    }

    QSGPainterNode *node = static_cast<QSGPainterNode *>(oldNode);
    if (!node) {
        node = d->sceneGraphContext()->createPainterNode(this);
        d->node = node;
    }

    bool hasTextureSize = d->textureSize.width() > 0 && d->textureSize.height() > 0;

    // Use the compat mode if any of the compat things are set and
    // textureSize is 0x0.
    if (!hasTextureSize
        && (d->contentsScale != 1
            || (d->contentsSize.width() > 0 && d->contentsSize.height() > 0))) {
        QRectF br = contentsBoundingRect();
        node->setContentsScale(d->contentsScale);
        QSize size = QSize(qRound(br.width()), qRound(br.height()));
        node->setSize(size);
        node->setTextureSize(size);
    } else {
        // The default, use textureSize or item's size * device pixel ratio
        node->setContentsScale(1);
        QSize itemSize(qRound(width()), qRound(height()));
        node->setSize(itemSize);
        QSize textureSize = (hasTextureSize ? d->textureSize : itemSize)
                            * window()->effectiveDevicePixelRatio();
        node->setTextureSize(textureSize);
    }

    node->setPreferredRenderTarget(d->renderTarget);
    node->setFastFBOResizing(d->performanceHints & FastFBOResizing);
    node->setSmoothPainting(d->antialiasing);
    node->setLinearFiltering(d->smooth);
    node->setMipmapping(d->mipmap);
    node->setOpaquePainting(d->opaquePainting);
    node->setFillColor(d->fillColor);
    node->setDirty(d->dirtyRect);
    node->update();

    d->dirtyRect = QRect();

    if (d->textureProvider) {
        d->textureProvider->node = node;
        d->textureProvider->fireTextureChanged();
    }

    return node;
}

/*!
   \reimp
*/
void QQuickPaintedItem::releaseResources()
{
    Q_D(QQuickPaintedItem);
    if (d->textureProvider) {
        QQuickWindowQObjectCleanupJob::schedule(window(), d->textureProvider);
        d->textureProvider = nullptr;
    }
    d->node = nullptr; // Managed by the scene graph, just clear the pointer.
}

void QQuickPaintedItem::invalidateSceneGraph()
{
    Q_D(QQuickPaintedItem);
    delete d->textureProvider;
    d->textureProvider = nullptr;
    d->node = nullptr; // Managed by the scene graph, just clear the pointer
}

/*!
   \reimp
*/
bool QQuickPaintedItem::isTextureProvider() const
{
    return true;
}

/*!
   \reimp
*/
QSGTextureProvider *QQuickPaintedItem::textureProvider() const
{
    // When Item::layer::enabled == true, QQuickItem will be a texture
    // provider. In this case we should prefer to return the layer rather
    // than the image itself. The layer will include any children and any
    // the image's wrap and fill mode.
    if (QQuickItem::isTextureProvider())
        return QQuickItem::textureProvider();

    Q_D(const QQuickPaintedItem);
    QQuickWindow *w = window();
    if (!w || !w->isSceneGraphInitialized() || QThread::currentThread() != d->sceneGraphContext()->thread()) {
        qWarning("QQuickPaintedItem::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }
    if (!d->textureProvider)
        d->textureProvider = new QQuickPaintedItemTextureProvider();
    d->textureProvider->node = d->node;
    return d->textureProvider;
}


/*!
   \reimp
*/
void QQuickPaintedItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemDevicePixelRatioHasChanged)
        update();
    QQuickItem::itemChange(change, value);
}

QT_END_NAMESPACE

#include "moc_qquickpainteditem.cpp"
