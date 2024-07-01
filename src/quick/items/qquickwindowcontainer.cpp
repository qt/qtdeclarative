// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindowcontainer_p.h"

#include <QtQuick/qquickrendercontrol.h>

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickwindowmodule_p.h>
#include <QtQuick/private/qquickimplicitsizeitem_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcWindowContainer, "qt.quick.window.container")

using namespace Qt::StringLiterals;

/*!
    \qmltype WindowContainer
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \inherits Item
    \since 6.7
    \preliminary

    \brief Allows embedding arbitrary QWindows into a Qt Quick scene.

    The window will become a child of the item's window,
    with its position, size, z-order, etc. managed by the item.

    Sibling items with a higher z-order than the window container
    will not automatically overlap the embedded window, as the
    window lives on top of the Qt Quick scene. To work around this,
    place the sibling items inside their own dedicated child window:

    \code
    Item {
        id: someItem
        WindowContainer {
            window: foreignWindow
        }
        Window {
            parent: someItem
            Item {
                id: siblingItem
            }
        }
    }
    \endcode

    Similarly, child Items of the window container will not automatically
    overlap the embedded window. To work around this, place the child
    item inside a dedicated child window.

    \code
    Item {
        id: someItem
        WindowContainer {
            id: windowContainer
            window: foreignWindow
            Window {
                parent: windowContainer
                Item {
                    id: childItem
                }
            }
        }
    }
    \endcode

    If positioning and sizing of a Window via anchors is required,
    the Window can be wrapped in a window container:

    \code
    Item {
        id: someItem
        WindowContainer {
            anchors.fill: parent
            window: Window {
                Item {
                }
            }
        }
    }
    \endcode

    \note The window container does not interoperate with QQuickWidget,
    QQuickWindow::setRenderTarget(), QQuickRenderControl, or similar
    functionality.

    \sa {QtQuick::Window::parent}
*/

/*!
    \qmlproperty QWindow QtQuick::WindowContainer::window

    This property holds the window to embed.
*/

class QQuickWindowContainerPrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickWindowContainer)
protected:
    bool transformChanged(QQuickItem *transformedItem) override;

public:
    QWindow *window = nullptr;
    QQuickWindowContainer::ContainerMode containerMode;
};

/*!
    \internal

    Creates a new window container.

    The container mode determines who has the last word in what the state
    of the contained window should be. If the window container is explicitly
    requested by the user via WindowContainer, the properties are set on the
    item, and the embedded window should match that. If the window container
    is implicitly created by setting a visual parent on a Window, the properties
    are set on the Window, and the window container should respect that.
*/
QQuickWindowContainer::QQuickWindowContainer(QQuickItem *parent, ContainerMode containerMode)
    : QQuickImplicitSizeItem(*(new QQuickWindowContainerPrivate), parent)
{
    Q_D(QQuickWindowContainer);

    qCDebug(lcWindowContainer).verbosity(1) << "Creating window container"
         << this << "with parent" << parent << "and" << containerMode;

    d->containerMode = containerMode;

    setFlag(QQuickItem::ItemObservesViewport); // For clipping

    connect(this, &QQuickItem::windowChanged,
            this, &QQuickWindowContainer::parentWindowChanged);

    if (lcWindowContainer().isDebugEnabled()) {
        auto *debugRectangle = new QQuickRectangle(this);
        debugRectangle->setColor(QColor(255, 0, 255, 20));
        auto *border = debugRectangle->border();
        border->setColor(Qt::magenta);
        border->setWidth(1.0);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(debugRectangle);
        rectPrivate->anchors()->setFill(this);
    }
}

QQuickWindowContainer::~QQuickWindowContainer()
{
    Q_D(const QQuickWindowContainer);
    qCDebug(lcWindowContainer) << "Destructing window container" << this;

    disconnect(this);
    if (d->window) {
        auto ownership = QJSEngine::objectOwnership(d->window);
        qCDebug(lcWindowContainer) << "Contained window" << d->window
            << "has" << (ownership == QQmlEngine::JavaScriptOwnership ?
                         "JavaScript" : "C++") << "ownership";
        if (ownership == QQmlEngine::JavaScriptOwnership) {
            delete d->window;
        } else {
            d->window->destroy();
            d->window->setParent(nullptr);
        }
    }
}

void QQuickWindowContainer::releaseResources()
{
    Q_D(const QQuickWindowContainer);
    qCDebug(lcWindowContainer) << "Destroying" << d->window
        << "with platform window" << (d->window ? d->window->handle() : nullptr);
    if (d->window)
        d->window->destroy();
}

void QQuickWindowContainer::classBegin()
{
    qCDebug(lcWindowContainer) << "Class begin for" << this;

    QQuickImplicitSizeItem::classBegin();
}

void QQuickWindowContainer::componentComplete()
{
    Q_D(const QQuickWindowContainer);

    qCDebug(lcWindowContainer) << "Component completed for" << this;
    QQuickImplicitSizeItem::componentComplete();

    if (d->window)
        initializeContainedWindow();
}

QWindow *QQuickWindowContainer::containedWindow() const
{
    Q_D(const QQuickWindowContainer);
    return d->window;
}

void QQuickWindowContainer::setContainedWindow(QWindow *window)
{
    qCDebug(lcWindowContainer) << "Setting contained window for" << this << "to" << window;

    Q_D(QQuickWindowContainer);

    if (window == d->window)
        return;

    if (auto *previousWindow = d->window) {
        qCDebug(lcWindowContainer) << "Decoupling container from" << d->window;
        previousWindow->disconnect(this);
        previousWindow->removeEventFilter(this);
        previousWindow->setParent(nullptr);
    }

    d->window = window;

    if (d->window) {
        if (d->containerMode == ItemControlsWindow) {
            if (auto *quickWindow = qobject_cast<QQuickWindowQmlImpl*>(d->window)) {
                // Make sure the Window reflects the window container as its visual parent
                quickWindow->setVisualParent(this);
            }
        }

        // When the window controls the container, we need to reflect any changes
        // in the window back to the container, so they stay in sync. And when the
        // container controls the window, we still want to reflect width/height as
        // new implicit size, and override any other changes with the item state.
        connect(d->window, &QWindow::xChanged, this, &QQuickWindowContainer::windowUpdated);
        connect(d->window, &QWindow::yChanged, this, &QQuickWindowContainer::windowUpdated);
        connect(d->window, &QWindow::widthChanged, this, &QQuickWindowContainer::windowUpdated);
        connect(d->window, &QWindow::heightChanged, this, &QQuickWindowContainer::windowUpdated);
        connect(d->window, &QWindow::visibleChanged, this, &QQuickWindowContainer::windowUpdated);

        connect(d->window, &QObject::destroyed, this, &QQuickWindowContainer::windowDestroyed);

        d->window->installEventFilter(this);

        if (d->componentComplete)
            initializeContainedWindow();
    } else {
        // Reset state based on not having a window
        syncWindowToItem();
    }

    emit containedWindowChanged(d->window);
}

void QQuickWindowContainer::initializeContainedWindow()
{
    Q_D(const QQuickWindowContainer);
    Q_ASSERT(d->componentComplete);
    Q_ASSERT(d->window);

    qCDebug(lcWindowContainer) << "Doing initial sync between" << d->window << "and" << this;

    syncWindowToItem();
    polish();
}

static QTransform sanitizeTransform(const QTransform &transform)
{
    if (transform.isRotating()) {
        // FIXME: Can we keep more here?
        return QTransform::fromTranslate(transform.dx(), transform.dy());
    }

    return transform;
}

void QQuickWindowContainer::syncWindowToItem()
{
    Q_D(const QQuickWindowContainer);

    const auto windowGeometry = d->window ? d->window->geometry() : QRect();

    qCDebug(lcWindowContainer) << "Syncing window state from" << d->window
        << "with geometry" << windowGeometry << "to" << this
        << "with mode" << d->containerMode;

    const auto transform = sanitizeTransform(d->windowToItemTransform());

    // The window might have a larger size than the item's natural
    // size, if there's a scale applied somewhere in the hierarchy.
    auto itemSize = d->window ? transform.mapRect(windowGeometry).size()
                              : QSize();

    if (d->containerMode == WindowControlsItem) {
        // When the Window controls the window container the position is
        // set up front, when creating the window container, and from that
        // point on set exclusively via the window container, so we skip
        // setting the position here, and only set the size.
        setSize(itemSize);
        setVisible(d->window ? d->window->isVisible() : false);
    } else {
        // Position defined by item, so don't sync from window
        // Visible defined by item, so don't sync from window
        setImplicitWidth(itemSize.width());
        setImplicitHeight(itemSize.height());
    }
}

/*!
    \internal

    updatePolish() should perform any layout as required for this item.

    For us, that means propagating the item's state to the window.
*/
void QQuickWindowContainer::updatePolish()
{
    Q_D(QQuickWindowContainer);

    qCDebug(lcWindowContainer) << "Propagating" << this << "state"
        << "to" << d->window;

    auto *parentWindow = window();

    // FIXME: If we are part of a QQuickWidget, we have a QQuickRenderControl,
    // and should look up the parent window via that, and apply the offset we
    // get to the item transform below. But at the moment it's not possible
    // to observe changes to the offset, which is critical to support this
    // for child windows.

    if (!d->window || !parentWindow)
        return;

    if (d->window->parent() != parentWindow) {
        qCDebug(lcWindowContainer) << "Updating window parent to" << parentWindow;
        d->window->setParent(parentWindow);
    }

    auto transform = sanitizeTransform(d->itemToWindowTransform());

    // Find the window's geometry, based on the item's bounding rect,
    // mapped to the scene. The mapping includes any x/y position set
    // on the item itself, as well as any transforms applied to the item
    // or its ancestor (scale, translation).
    const QRectF itemSceneRect = transform.mapRect(boundingRect());
    // FIXME: Rounding to a QRect here means we'll have some jitter or off
    // placement when the underlying item is not on a integer coordinate.
    QRect windowGeometry = itemSceneRect.toRect();
    if (windowGeometry != d->window->geometry()) {
        QRectF itemRect(position(), size());
        qCDebug(lcWindowContainer) << "Updating window geometry to" << windowGeometry
            << "based on item rect" << itemRect << "and scene rect" << itemSceneRect;
        d->window->setGeometry(windowGeometry);
    }

    // Clip the container to its own and ancestor clip rects, by setting
    // a mask on the window. This does not necessarily clip native windows,
    // as QWindow::setMask() is not guaranteed to visually clip the window,
    // only to mask input, but in most cases we should be good. For the
    // cases where this fails, we can potentially use an intermediate window
    // as parent of the contained window, if the platform allows clipping
    // child windows to parent window geometry. We do not want to resize the
    // contained window, as that will just fill the content into a smaller
    // area.
    const auto clipMask = [&]{
        if (clipRect() == boundingRect())
            return QRect();

        // The clip rect has all the relevant transforms applied to it,
        // except for the item's own scale. As the mask is in window
        // local coordinates in the possibly scaled window, we need
        // to apply the scale manually.
        auto scaleTransform = QTransform::fromScale(transform.m11(), transform.m22());
        auto rect = scaleTransform.mapRect(clipRect()).toRect();

        // An empty clip rect means clip away everything, while for a
        // window, an empty mask means mask nothing. Fake the former
        // by setting a mask outside of the window's bounds. We have
        // to do this check after rounding the clip rect to a QRect.
        // FIXME: Verify this works on all platforms
        if (rect.isEmpty())
            return QRect(-1, -1, 1, 1);

        return rect;
    }();

    if (clipMask != d->window->mask().boundingRect()) {
        qCDebug(lcWindowContainer) << "Updating window clip mask to" << clipMask
            << "based on clip rect" << clipRect();
        d->window->setMask(clipMask);
    }

    // FIXME: Opacity support. Need to calculate effective opacity ourselves,
    // and there doesn't seem to be any existing observer for opacity changes.
    // Not all platforms implement opacity for child windows yet.

    // FIXME: If a scale is applied to the item or its parents, we end up
    // with a bigger item, and window, but we don't translate the scale to
    // an increase device-pixel-ratio of the window. As a result, the window
    // will likely just render more content, instead of the same content at
    // a potentially higher density.

    if (d->window->isVisible() != isVisible()) {
        qCDebug(lcWindowContainer) << "Updating window visibility"
            << "based on item visible" << isVisible();
        d->window->setVisible(isVisible());
    }
}

/*!
    \internal

    QQuickItem::clipRect() doesn't take ItemClipsChildrenToShape into
    account, so a parent item that has clip:false, but ItemIsViewport
    will still result in affecting the clip.

    We want to stay consistent with the clipping in the scene graph,
    which is based on QQuickItem::clip(), so we override the clipRect
    to take ItemClipsChildrenToShape into account.
*/
QRectF QQuickWindowContainer::clipRect() const
{
    QRectF rect = boundingRect();

    for (auto *viewport = viewportItem(); viewport; viewport = viewport->viewportItem()) {
        if (viewport == this)
            break;

        if (viewport->flags().testFlag(QQuickItem::ItemClipsChildrenToShape)) {
            // FIXME: This fails to take into account viewports that override clipRect()
            const auto mappedViewportRect = mapRectFromItem(viewport, viewport->boundingRect());
            rect = mappedViewportRect.intersected(rect);
        }

        if (viewport->viewportItem() == viewport)
            break; // Content item returns itself as viewport
    }

    return rect;
}

// ----------------------- Window updates -----------------------

/*!
    \internal

    Called when the contained QWindow is changed.

    Depending on the sync mode we need to reflect these changes
    to the item, or override them by applying the item state.
*/
void QQuickWindowContainer::windowUpdated()
{
    Q_D(const QQuickWindowContainer);

    if (lcWindowContainer().isDebugEnabled()) {
        auto metaMethod = sender()->metaObject()->method(senderSignalIndex());
        auto signalName = QString::fromUtf8(metaMethod.name());
        qCDebug(lcWindowContainer).noquote() << d->window << signalName;
    }

    syncWindowToItem();

    if (d->containerMode == ItemControlsWindow) {
        qCDebug(lcWindowContainer) << "Overriding window state by polishing";
        // Ideally we'd always call ensurePolished() here, to synchronously
        // override the window state ASAP, rather than wait for polish to
        // trigger it asynchronously, but due to QWindowPrivate::setVisible
        // emitting visibleChanged before updating the platform window, we
        // end up applying our override temporarily, only to have QWindowPrivate
        // follow up with the original change to the platform window.
        if (d->window->isVisible() != isVisible())
            polish();
        else
            ensurePolished();
    }
}

bool QQuickWindowContainer::eventFilter(QObject *object, QEvent *event)
{
    Q_D(const QQuickWindowContainer);
    Q_ASSERT(object == d->window);

    if (event->type() == QEvent::PlatformSurface) {
        auto type = static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType();
        if (type == QPlatformSurfaceEvent::SurfaceCreated) {
            qCDebug(lcWindowContainer) << "Surface created for" << object;
            syncWindowToItem();
            // The surface creation has already resulted in the native window
            // being added to its parent, on top of all other windows. We need
            // to do a synchronous re-stacking of the windows here, to avoid
            // leaving the window in the wrong position while waiting for the
            // asynchronous callback to QQuickWindow::polishItems().
            if (auto *quickWindow = qobject_cast<QQuickWindow*>(window()))
                QQuickWindowPrivate::get(quickWindow)->updateChildWindowStackingOrder();
        }
    }

    return QQuickImplicitSizeItem::eventFilter(object, event);
}

void QQuickWindowContainer::windowDestroyed()
{
    Q_D(QQuickWindowContainer);
    qCDebug(lcWindowContainer) << "Window" << (void*)d->window << "destroyed";

    d->window->removeEventFilter(this);
    d->window = nullptr;

    syncWindowToItem(); // Reset state based on not having a window
    emit containedWindowChanged(d->window);
}

// ----------------------- Item updates -----------------------

/*!
    \internal

    Called when the item's geometry has changed
*/
void QQuickWindowContainer::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    qCDebug(lcWindowContainer) << this << "geometry changed from"
        << oldGeometry << "to" << newGeometry;

    QQuickImplicitSizeItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.isValid())
        polish();
}

/*!
    \internal

    Called when the item's (effective) state has changed
*/
void QQuickWindowContainer::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    switch (change) {
    case ItemVisibleHasChanged:
        qCDebug(lcWindowContainer) << "Visible changed for" << this << "to" << isVisible();
        polish();
        break;
    default:
        break;
    }

    QQuickImplicitSizeItem::itemChange(change, data);
}

/*!
    \internal

    Called when the window container item is moved to another window
*/
void QQuickWindowContainer::parentWindowChanged(QQuickWindow *parentWindow)
{
    qCDebug(lcWindowContainer) << this << "parent window changed to" << parentWindow;

    Q_D(QQuickWindowContainer);

    if (!parentWindow) {
        // We have been removed from the window we were part of,
        // possibly because the window is going away. We need to
        // make sure the contained window is no longer a child of
        // former window, as otherwise it will be wiped out along
        // with it. We can't wait for updatePolish() to do that
        // as polish has no effect when an item is not part of a
        // window.
        if (d->window) {
            // The window should already be destroyed from the
            // call to releaseResources(), which is part of the
            // removal of an item from a scene, but just in case
            // we do it here as well.
            d->window->destroy();

            d->window->setParent(nullptr);
        }
    } else {
        polish();
    }
}

bool QQuickWindowContainerPrivate::transformChanged(QQuickItem *transformedItem)
{
    Q_Q(QQuickWindowContainer);

    if (this->componentComplete && this->window) {
        auto *transformedItemPrivate = QQuickItemPrivate::get(transformedItem);
        qCDebug(lcWindowContainer) << "Transform changed for" << transformedItem
            << "with dirty state" << transformedItemPrivate->dirtyToString();

        if (transformedItemPrivate->dirtyAttributes
            & QQuickItemPrivate::BasicTransform) {
            // For some reason scale transforms, which result in the window
            // being resized, end up with the window lagging a frame or two
            // behind the item. Polish synchronously instead, to mitigate
            // this, even if it may result in the opposite situation.
            q->ensurePolished();
        } else {
            q->polish();
        }
    }

    return QQuickItemPrivate::transformChanged(transformedItem);
}

QT_END_NAMESPACE
