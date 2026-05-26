// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <private/qtquickglobal_p.h>
#include "qquickitemgrabresult.h"

#include "qquickrendercontrol.h"
#include "qquickwindow.h"
#include "qquickitem.h"
#if QT_CONFIG(quick_shadereffect)
#include "qquickshadereffectsource_p.h"
#endif

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlInfo>

#include <private/qquickpixmap_p.h>
#include <private/qquickitem_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

const QEvent::Type Event_Grab_Completed = static_cast<QEvent::Type>(QEvent::User + 1);

class QQuickItemGrabResultPrivate : public QObjectPrivate
{
public:
    QQuickItemGrabResultPrivate()
        : cacheEntry(nullptr)
        , qmlEngine(nullptr)
        , texture(nullptr)
        , devicePixelRatio(1.0)
    {
    }

    ~QQuickItemGrabResultPrivate()
    {
        // Remove the unmanaged heap size we've added when creating the image in
        // QQuickItemGrabResult::render(). If render() was never called the image is empty and
        // its size is 0.
        if (hasCallback())
            qmlEngine->handle()->memoryManager->changeUnmanagedHeapSizeUsage(-image.sizeInBytes());
        delete cacheEntry;
    }

    // Should be called under a locked mutex!
    bool hasCallback() const { return qmlEngine && callback.isCallable(); }

    // Should be called under a locked mutex!
    void ensureImageInCache() const {
        if (url.isEmpty() && !image.isNull()) {
            url.setScheme(QQuickPixmap::itemGrabberScheme);
            url.setPath(QVariant::fromValue(item.data()).toString());
            static uint counter = 0;
            url.setFragment(QString::number(++counter));
            cacheEntry = new QQuickPixmap(url, image);
        }
    }

    static QQuickItemGrabResult *create(QQuickItem *item, const QSize &size);

    mutable QBasicMutex mutex;

    QImage image;

    mutable QUrl url;
    mutable QQuickPixmap *cacheEntry;

    QQmlEngine *qmlEngine;
    QJSValue callback;

    QPointer<QQuickItem> item;
    QPointer<QQuickWindow> window;
    QSGLayer *texture;
    QSizeF itemSize;
    QSize textureSize;
    qreal devicePixelRatio;

    QMetaObject::Connection setupConnection;
    QMetaObject::Connection renderConnection;
};

/*!
 * \qmlproperty url QtQuick::ItemGrabResult::url
 *
 * This property holds a URL which can be used in conjunction with
 * URL based image consumers, such as the QtQuick::Image type.
 *
 * The URL is valid while there is a reference in QML or JavaScript
 * to the ItemGrabResult or while the image the URL references is
 * actively used.
 *
 * The URL does not represent a valid file or location to read it from, it
 * is primarily a key to access images through Qt Quick's image-based types.
 */

/*!
 * \property QQuickItemGrabResult::url
 *
 * This property holds a URL which can be used in conjunction with
 * URL based image consumers, such as the QtQuick::Image type.
 *
 * The URL is valid until the QQuickItemGrabResult object is deleted.
 *
 * The URL does not represent a valid file or location to read it from, it
 * is primarily a key to access images through Qt Quick's image-based types.
 */

/*!
 * \qmlproperty variant QtQuick::ItemGrabResult::image
 *
 * This property holds the pixel results from a grab in the
 * form of a QImage.
 */

/*!
 * \property QQuickItemGrabResult::image
 *
 * This property holds the pixel results from a grab.
 *
 * If the grab is not yet complete or if it failed,
 * a null image is returned (\c {image.isNull()} will return \c true).
 */

/*!
    \class QQuickItemGrabResult
    \inmodule QtQuick
    \brief The QQuickItemGrabResult contains the result from QQuickItem::grabToImage().

    \sa QQuickItem::grabToImage()
 */

/*!
 * \fn void QQuickItemGrabResult::ready()
 *
 * This signal is emitted when the grab has completed.
 */

/*!
 * \qmltype ItemGrabResult
 * \nativetype QQuickItemGrabResult
 * \inherits QtObject
 * \inqmlmodule QtQuick
 * \ingroup qtquick-visual
 * \brief Contains the results from a call to Item::grabToImage().
 *
 * The ItemGrabResult is a small container used to encapsulate
 * the results from Item::grabToImage().
 *
 * \sa Item::grabToImage()
 */

QQuickItemGrabResult::QQuickItemGrabResult(QObject *parent)
    : QObject(*new QQuickItemGrabResultPrivate, parent)
{
}

/*!
 * \qmlmethod bool QtQuick::ItemGrabResult::saveToFile(fileName)
 *
 * Saves the grab result as an image to \a fileName. Returns \c true
 * if successful; otherwise returns \c false.
 */

// ### Qt 7: remove and keep only QUrl overload
/*!
 * Saves the grab result as an image to \a fileName. Returns \c true
 * if successful; otherwise returns \c false.
 *
 * \note In Qt versions prior to 5.9, this function is marked as non-\c{const}.
 */
bool QQuickItemGrabResult::saveToFile(const QString &fileName) const
{
    Q_D(const QQuickItemGrabResult);
    if (fileName.startsWith(QLatin1String("file:/")))
        return saveToFile(QUrl(fileName));
    QMutexLocker locker(&d->mutex);
    return d->image.save(fileName);
}

/*!
 * \since 6.2
 * Saves the grab result as an image to \a filePath, which must refer to a
 * \l{QUrl::isLocalFile}{local file name} with a
 * \l{QImageWriter::supportedImageFormats()}{supported image format} extension.
 * Returns \c true if successful; otherwise returns \c false.
 */
bool QQuickItemGrabResult::saveToFile(const QUrl &filePath) const
{
    Q_D(const QQuickItemGrabResult);
    if (!filePath.isLocalFile()) {
        qWarning() << "saveToFile can only save to a file on the local filesystem";
        return false;
    }
    QMutexLocker locker(&d->mutex);
    return d->image.save(filePath.toLocalFile());
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if QT_DEPRECATED_SINCE(5, 15)
/*!
 * \overload
 * \internal
 */
bool QQuickItemGrabResult::saveToFile(const QString &fileName)
{
    return std::as_const(*this).saveToFile(fileName);
}
#endif
#endif // < Qt 6

QUrl QQuickItemGrabResult::url() const
{
    Q_D(const QQuickItemGrabResult);
    QMutexLocker locker(&d->mutex);
    d->ensureImageInCache();
    return d->url;
}

QImage QQuickItemGrabResult::image() const
{
    Q_D(const QQuickItemGrabResult);
    QMutexLocker locker(&d->mutex);
    return d->image;
}

/*!
 * \internal
 */
bool QQuickItemGrabResult::event(QEvent *e)
{
    Q_D(QQuickItemGrabResult);
    if (e->type() == Event_Grab_Completed) {
        // We cannot hold the mutex while emitting the signal or calling the
        // callback, because they can immediately call various getters.
        QJSValue callback;
        QQmlEngine *engine = nullptr;
        if (QMutexLocker locker(&d->mutex); d->hasCallback()) {
            callback = d->callback;
            engine = d->qmlEngine;
        }
        if (engine) {
            // We have a JS callback. Transfer ownership to JavaScript to follow the documentation.
            // The user should be able to store the object in QML, but we also want to GC it
            // eventually.
            QQmlEngine::setObjectOwnership(this, QQmlEngine::JavaScriptOwnership);
            setParent(nullptr);
            callback.call(QJSValueList() << engine->newQObject(this));
        } else {
            Q_EMIT ready();
        }
        return true;
    }
    return QObject::event(e);
}

/*!
    \internal
    This method is called from QSGRenderThread
*/
void QQuickItemGrabResult::setup()
{
    Q_D(QQuickItemGrabResult);
    QMutexLocker locker(&d->mutex);
    if (!d->item) {
        disconnect(d->setupConnection);
        disconnect(d->renderConnection);
        QCoreApplication::postEvent(this, new QEvent(Event_Grab_Completed));
        return;
    }

    QSGRenderContext *rc = QQuickWindowPrivate::get(d->window.data())->context;
    d->devicePixelRatio = d->window->effectiveDevicePixelRatio();
    d->texture = rc->sceneGraphContext()->createLayer(rc);
    d->texture->setDevicePixelRatio(d->devicePixelRatio);
    d->texture->setItem(QQuickItemPrivate::get(d->item)->itemNode());
    d->itemSize = QSizeF(d->item->width(), d->item->height());
}

/*!
    \internal
    This method is called from QSGRenderThread
*/
void QQuickItemGrabResult::render()
{
    Q_D(QQuickItemGrabResult);
    QMutexLocker locker(&d->mutex);
    if (!d->texture)
        return;

    d->texture->setRect(QRectF(0, d->itemSize.height(), d->itemSize.width(), -d->itemSize.height()));
    const QSize minSize = QQuickWindowPrivate::get(d->window.data())->context->sceneGraphContext()->minimumFBOSize();
    const QSize effectiveTextureSize = d->textureSize * d->devicePixelRatio;
    d->texture->setSize(QSize(qMax(minSize.width(), effectiveTextureSize.width()),
                              qMax(minSize.height(), effectiveTextureSize.height())));
    d->texture->scheduleUpdate();
    d->texture->updateTexture();

    const qsizetype oldImageSize = d->image.sizeInBytes();
    d->image = d->texture->toImage();
    d->image.setDevicePixelRatio(d->devicePixelRatio);
    const qsizetype newImageSize = d->image.sizeInBytes();
    if (d->hasCallback()) {
        // If we have a callback, we assume that it's going to be called, eventually. If you post
        // an event that never arrives, you have worse problems. Based on that assumption, we track
        // the (potentially large) image as unmanaged heap size already now. This is the only place
        // where we can determine the size. In the dtor we need to "untrack" the image if it was
        // tracked. So either we need to store an extra bit that tells us whether the callback was
        // called or we need to live with stale unmanaged heap size in the unlikely case that it's
        // never called. We choose the latter.
        d->qmlEngine->handle()->memoryManager->changeUnmanagedHeapSizeUsage(
                newImageSize - oldImageSize);
    }

    delete d->texture;
    d->texture = nullptr;

    disconnect(d->setupConnection);
    disconnect(d->renderConnection);
    QCoreApplication::postEvent(this, new QEvent(Event_Grab_Completed));
}

QQuickItemGrabResult *QQuickItemGrabResultPrivate::create(QQuickItem *item, const QSize &targetSize)
{
    QSize size = targetSize;
    if (size.isEmpty())
        size = QSize(item->width(), item->height());

    if (size.width() < 1 || size.height() < 1) {
        qmlWarning(item) << "grabToImage: item's width and/or height are less than 1: " << size;
        return nullptr;
    }

    if (!item->window()) {
        qmlWarning(item) << "grabToImage: item is not attached to a window";
        return nullptr;
    }

    QWindow *effectiveWindow = item->window();
    if (QWindow *renderWindow = QQuickRenderControl::renderWindowFor(item->window()))
        effectiveWindow = renderWindow;

    if (!effectiveWindow->isVisible()) {
        qmlWarning(item) << "grabToImage: item's window is not visible";
        return nullptr;
    }

    // Initially parent to item so that we don't leak it if Event_Grab_Completed never arrives or
    // the callback can't be called.
    QQuickItemGrabResult *result = new QQuickItemGrabResult(item);
    QQuickItemGrabResultPrivate *d = result->d_func();
    // no need to lock here, since we didn't do the connection yet, so it cannot be
    // accessed from multiple threads
    d->item = item;
    d->window = item->window();
    d->textureSize = size;

    QQuickItemPrivate::get(item)->refFromEffectItem(false);

    // trigger sync & render
    item->window()->update();

    return result;
}

/*!
 * Grabs the item into an in-memory image.
 *
 * The grab happens asynchronously and the signal QQuickItemGrabResult::ready()
 * is emitted when the grab has been completed.
 *
 * Use \a targetSize to specify the size of the target image. By default, the
 * result will have the same size as item.
 *
 * If the grab could not be initiated, the function returns \c null.
 *
 * \note This function will render the item to an offscreen surface and
 * copy that surface from the GPU's memory into the CPU's memory, which can
 * be quite costly. For "live" preview, use \l {QtQuick::Item::layer.enabled} {layers}
 * or ShaderEffectSource.
 *
 * \sa QQuickWindow::grabWindow()
 */
QSharedPointer<QQuickItemGrabResult> QQuickItem::grabToImage(const QSize &targetSize)
{
    QQuickItemGrabResult *result = QQuickItemGrabResultPrivate::create(this, targetSize);
    if (!result)
        return QSharedPointer<QQuickItemGrabResult>();

    QQuickItemGrabResultPrivate *d = result->d_func();
    auto sharedResult = QSharedPointer<QQuickItemGrabResult>(result);
    result->setParent(nullptr); // the smart pointer now manages the lifetime

    auto weakResult = QWeakPointer(sharedResult);
    QMutexLocker locker(&d->mutex);
    d->setupConnection = connect(window(), &QQuickWindow::beforeSynchronizing,
                                 result, [weakResult] {
                                     if (auto strong = weakResult.toStrongRef(); strong)
                                         strong->setup();
                                 }, Qt::DirectConnection);
    d->renderConnection = connect(window(), &QQuickWindow::afterRendering,
                                  result, [weakResult] {
                                      if (auto strong = weakResult.toStrongRef(); strong)
                                          strong->render();
                                  }, Qt::DirectConnection);

    return sharedResult;
}

/*!
 * \qmlmethod bool QtQuick::Item::grabToImage(callback, targetSize)
 *
 * Grabs the item into an in-memory image.
 *
 * The grab happens asynchronously and the JavaScript function \a callback is
 * invoked when the grab is completed. The callback takes one argument, which
 * is the result of the grab operation; an \l ItemGrabResult object.
 *
 * Use \a targetSize to specify the size of the target image. By default, the result
 * will have the same size as the item.
 *
 * If the grab could not be initiated, the function returns \c false.
 *
 * The following snippet shows how to grab an item and store the results in
 * a file:
 *
 * \snippet qml/item/itemGrab.qml grab-to-file
 *
 * The following snippet shows how to grab an item and use the results in
 * another image element:
 *
 * \snippet qml/item/itemGrab.qml grab-to-image
 *
 * \note This function will render the item to an offscreen surface and
 * copy that surface from the GPU's memory into the CPU's memory, which can
 * be quite costly. For "live" preview, use \l {QtQuick::Item::layer.enabled} {layers}
 * or ShaderEffectSource.
 */

/*!
 * \internal
 * Only visible from QML.
 */
bool QQuickItem::grabToImage(const QJSValue &callback, const QSize &targetSize)
{
    QQmlEngine *engine = qmlEngine(this);
    if (!engine) {
        qmlWarning(this) << "grabToImage: item has no QML engine";
        return false;
    }

    if (!callback.isCallable()) {
        qmlWarning(this) << "grabToImage: 'callback' is not a function";
        return false;
    }

    QSize size = targetSize;
    if (size.isEmpty())
        size = QSize(width(), height());

    if (size.width() < 1 || size.height() < 1) {
        qmlWarning(this) << "grabToImage: item's width and/or height are less than 1: " << size;
        return false;
    }

    if (!window()) {
        qmlWarning(this) << "grabToImage: item is not attached to a window";
        return false;
    }

    QQuickItemGrabResult *result = QQuickItemGrabResultPrivate::create(this, size);
    if (!result)
        return false;

    // Do not transfer ownership to JavaScript here, yet. There is no reference to the object
    // We don't want the GC to collect it while the grab is still in flight.
    QQuickItemGrabResultPrivate *d = result->d_func();

    QMutexLocker locker(&d->mutex);
    d->qmlEngine = engine;
    d->callback = callback;

    d->setupConnection = connect(window(), &QQuickWindow::beforeSynchronizing,
                                 result, &QQuickItemGrabResult::setup, Qt::DirectConnection);
    d->renderConnection = connect(window(), &QQuickWindow::afterRendering,
                                  result, &QQuickItemGrabResult::render, Qt::DirectConnection);

    return true;
}

QT_END_NAMESPACE

#include "moc_qquickitemgrabresult.cpp"
