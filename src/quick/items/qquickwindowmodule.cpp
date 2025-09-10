// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindowmodule_p.h"
#include "qquickwindowattached_p.h"
#include "qquickrendercontrol.h"
#include "qquickscreen_p.h"
#include "qquickview_p.h"
#include "qquickwindowmodule_p_p.h"
#include "qquickitem_p.h"
#include <QtQuick/QQuickWindow>
#include <QtCore/QCoreApplication>
#include <QtQml/QQmlEngine>

#include <private/qguiapplication_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlglobal_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcTransient)

QQuickWindowQmlImplPrivate::QQuickWindowQmlImplPrivate() = default;

QQuickWindowQmlImpl::QQuickWindowQmlImpl(QWindow *parent)
    : QQuickWindowQmlImpl(*(new QQuickWindowQmlImplPrivate), parent)
{
}

QQuickWindowQmlImpl::QQuickWindowQmlImpl(QQuickWindowQmlImplPrivate &dd, QWindow *parent)
    : QQuickWindow(dd, parent)
{
    connect(this, &QWindow::visibleChanged, this, [&] {
        Q_D(QQuickWindowQmlImpl);
        d->visible = QWindow::isVisible();
        emit QQuickWindowQmlImpl::visibleChanged(d->visible);
    });
    connect(this, &QWindow::visibilityChanged, this, [&]{
        Q_D(QQuickWindowQmlImpl);
        // Update the window's actual visibility and turn off visibilityExplicitlySet,
        // so that future applyWindowVisibility() calls do not apply both window state
        // and visible state, unless setVisibility() is called again by the user.
        d->visibility = QWindow::visibility();
        d->visibilityExplicitlySet = false;
        emit QQuickWindowQmlImpl::visibilityChanged(d->visibility);
    });
    connect(this, &QWindow::screenChanged, this, &QQuickWindowQmlImpl::screenChanged);

    // We shadow the x and y properties, so that we can re-map them in case
    // we have an Item as our visual parent, which will result in creating an
    // implicit window container that we control. Ensure that signals still work,
    // and that they reflect the mapped values if a window container is used.
    QObject::connect(this, &QWindow::xChanged, this, [this] { emit xChanged(x()); });
    QObject::connect(this, &QWindow::yChanged, this, [this] { emit yChanged(y()); });
}

QQuickWindowQmlImpl::~QQuickWindowQmlImpl()
{
    // Destroy the window while we are still alive, so that any signals
    // emitted by the destruction can be delivered properly.
    destroy();
}

void QQuickWindowQmlImpl::classBegin()
{
    Q_D(QQuickWindowQmlImpl);
    qCDebug(lcQuickWindow) << "Class begin for" << this;
    d->componentComplete = false;

    QQmlEngine* e = qmlEngine(this);

    QQmlEngine::setContextForObject(contentItem(), e->rootContext());

    //Give QQuickView behavior when created from QML with QQmlApplicationEngine
    if (QCoreApplication::instance()->property("__qml_using_qqmlapplicationengine") == QVariant(true)) {
        if (e && !e->incubationController())
            e->setIncubationController(incubationController());
    }
    {
        // The content item has CppOwnership policy (set in QQuickWindow). Ensure the presence of a JS
        // wrapper so that the garbage collector can see the policy.
        QV4::ExecutionEngine *v4 = e->handle();
        QV4::QObjectWrapper::ensureWrapper(v4, d->contentItem);
    }
}

void QQuickWindowQmlImpl::componentComplete()
{
    Q_D(QQuickWindowQmlImpl);
    qCDebug(lcQuickWindow) << "Component completed for" << this;
    d->componentComplete = true;

    applyVisualParent();

    // Apply automatic transient parent if needed, and opt in to future
    // parent change events, so we can keep the transient parent in sync.
    updateTransientParent();
    d->receiveParentEvents = true;

    applyWindowVisibility();

    // If the transient parent changes, and we've deferred making
    // the window visible, we need to re-evaluate our decision.
    connect(this, &QWindow::transientParentChanged,
            this, &QQuickWindowQmlImpl::applyWindowVisibility);
}

void QQuickWindowQmlImpl::setVisible(bool visible)
{
    Q_D(QQuickWindowQmlImpl);
    d->visible = visible;
    d->visibleExplicitlySet = true;
    if (d->componentComplete)
        applyWindowVisibility();
}

void QQuickWindowQmlImpl::setVisibility(Visibility visibility)
{
    Q_D(QQuickWindowQmlImpl);
    d->visibility = visibility;
    d->visibilityExplicitlySet = true;
    if (d->componentComplete)
        applyWindowVisibility();
}

bool QQuickWindowQmlImpl::event(QEvent *event)
{
    Q_D(QQuickWindowQmlImpl);

    if (event->type() == QEvent::ParentWindowChange) {
        qCDebug(lcQuickWindow) << "Parent of" << this << "changed to" << parent();
        if (d->visualParent) {
            // If the window parent changes, and we've deferred making
            // the window visible, we need to re-evaluate our decision.
            applyWindowVisibility();
        } else {
            QObject::disconnect(d->itemParentWindowChangeListener);
            updateTransientParent();
        }
    }
    return QQuickWindow::event(event);
}

/*
    Update the transient parent of the window based on its
    QObject parent (Item or Window), unless the user has
    set an explicit transient parent.
*/
void QQuickWindowQmlImpl::updateTransientParent()
{
    Q_D(QQuickWindowQmlImpl);

    // We defer updating the transient parent until the component
    // has been fully completed, and we know whether an explicit
    // transient parent has been set.
    if (!d->componentComplete)
        return;

    // If an explicit transient parent has been set,
    // we don't want to apply our magic.
    if (d->transientParentPropertySet)
        return;

    // Nor if we have a visual parent that makes this a true child window
    if (d->visualParent)
        return;

    auto *objectParent = QObject::parent();
    qCDebug(lcTransient) << "Applying transient parent magic to"
        << this << "based on object parent" << objectParent << "🪄";

    QWindow *transientParent = nullptr;
    if (auto *windowParent = qmlobject_cast<QWindow *>(objectParent)) {
        transientParent = windowParent;
    } else if (auto *itemParent = qmlobject_cast<QQuickItem *>(objectParent)) {
        if (!d->itemParentWindowChangeListener) {
            d->itemParentWindowChangeListener = connect(
                itemParent, &QQuickItem::windowChanged,
                this, &QQuickWindowQmlImpl::updateTransientParent);
        }
        transientParent = itemParent->window();
    }

    if (!transientParent) {
        qCDebug(lcTransient) << "No transient parent resolved from object parent";
        return;
    }

    qCDebug(lcTransient) << "Setting" << transientParent << "as transient parent of" << this;
    setTransientParent(transientParent);

    // We want to keep applying the automatic transient parent
    d->transientParentPropertySet = false;
}

void QQuickWindowQmlImpl::applyWindowVisibility()
{
    Q_D(QQuickWindowQmlImpl);

    Q_ASSERT(d->componentComplete);

    const bool visible = d->visibilityExplicitlySet
        ? d->visibility != Hidden : d->visible;

    qCDebug(lcQuickWindow) << "Applying visible" << visible << "for" << this;

    if (visible) {
        if (d->visualParent) {
            // Even though we're complete, and have a visual parent set,
            // we may not be part of a window yet, or we may have been
            // removed from a window that's going away. Showing this window
            // now would make it a top level, which is not what we want.
            if (!QWindow::parent()) {
                qCDebug(lcQuickWindow) << "Waiting for visual parent to reparent us into a window";
                // We apply the visibility again on ParentWindowChange
                return;
            }
        } else {
            // Handle deferred visibility due to possible transient parent
            auto *itemParent = qmlobject_cast<QQuickItem *>(QObject::parent());
            if (!d->transientParentPropertySet && itemParent && !itemParent->window()) {
                qCDebug(lcTransient) << "Waiting for parent" << itemParent << "to resolve"
                                     << "its window. Deferring visibility";
                return;
            }

            const QWindow *transientParent = QWindow::transientParent();
            if (transientParent && !transientParentVisible()) {
                // Defer visibility of this window until the transient parent has
                // been made visible, or we've get a new transient parent.
                qCDebug(lcTransient) << "Transient parent" << transientParent
                    << "not visible yet. Deferring visibility";

                // QWindowPrivate::setVisible emits visibleChanged _before_ actually
                // propagating the visibility to the platform window, so we can't use
                // a direct connection here, as that would result in showing this
                // window before the transient parent.
                connect(transientParent, &QQuickWindow::visibleChanged, this,
                        &QQuickWindowQmlImpl::applyWindowVisibility,
                        Qt::ConnectionType(Qt::QueuedConnection | Qt::SingleShotConnection));
                return;
            }
        }
    }

    if (d->visibleExplicitlySet && d->visibilityExplicitlySet &&
        ((d->visibility == Hidden && d->visible) ||
         (d->visibility > AutomaticVisibility && !d->visible))) {
        // FIXME: Should we bail out in this case?
        qmlWarning(this) << "Conflicting properties 'visible' and 'visibility'";
    }

    if (d->visibility == AutomaticVisibility) {
        // We're either showing for the first time, with the default
        // visibility of AutomaticVisibility, or the user has called
        // setVisibility with AutomaticVisibility at some point, so
        // apply both window state and visible.
        if (QWindow::parent() || visualParent())
            setWindowState(Qt::WindowNoState);
        else
            setWindowState(QGuiApplicationPrivate::platformIntegration()->defaultWindowState(flags()));
        QQuickWindow::setVisible(d->visible);
    } else if (d->visibilityExplicitlySet) {
        // We're not AutomaticVisibility, but the user has requested
        // an explicit visibility, so apply both window state and visible.
        QQuickWindow::setVisibility(d->visibility);
    } else {
        // Our window state should be up to date, so only apply visible
        QQuickWindow::setVisible(d->visible);
    }
}

bool QQuickWindowQmlImpl::transientParentVisible()
{
   Q_ASSERT(transientParent());
   if (!transientParent()->isVisible()) {
       // handle case where transient parent is offscreen window
       QWindow *rw = QQuickRenderControl::renderWindowFor(qobject_cast<QQuickWindow*>(transientParent()));
       return rw && rw->isVisible();
   }
   return true;
}

// -------------------------- Visual Parent ---------------------------

/*!
    \qmlproperty var QtQuick::Window::parent
    \since 6.7
    \internal

    This property holds the visual parent of the window.

    The visual parent can be either another Window, or an Item.

    A window with a visual parent will result in the window becoming a child
    window of its visual parent, either directly if the visual parent is another
    Window, or indirectly via the visual parent Item's window.

    Just like QtQuick::Item::parent, the window will be positioned relative to
    its visual parent.

    The stacking order between sibling Windows follows the document order,
    just like Items, but can be customized via the Window's \l{QtQuick::Window::z}
    {z-order} property.

    Setting a visual parent on a Window will take precedence over the
    \l{QtQuick::Window::transientParent}{transient parent}.

    \sa{Concepts - Visual Parent in Qt Quick}, transientParent
*/

void QQuickWindowQmlImpl::setVisualParent(QObject *visualParent)
{
    Q_D(QQuickWindowQmlImpl);
    if (visualParent == d->visualParent)
        return;

    qCDebug(lcQuickWindow) << "Setting visual parent of" << this << "to" << visualParent;

    if (d->visualParent) {
        // Disconnect from deferred window listener
        d->visualParent->disconnect(this);
    }

    d->visualParent = visualParent;

    if (d->componentComplete)
        applyVisualParent();

    emit visualParentChanged(d->visualParent);
}

void QQuickWindowQmlImpl::applyVisualParent()
{
    Q_D(QQuickWindowQmlImpl);
    Q_ASSERT(d->componentComplete);

    qCDebug(lcQuickWindow) << "Applying" << this << "visual parent" << d->visualParent;

    if (!d->visualParent) {
        if (d->windowContainer) {
            d->windowContainer->setContainedWindow(nullptr);
            delete std::exchange(d->windowContainer, nullptr);
        }
        QQuickWindow::setParent(nullptr);
        return;
    }

    QQuickItem *parentItem = nullptr;
    if ((parentItem = qobject_cast<QQuickItem*>(d->visualParent)))
        ; // All good, can use directly
    else if (auto *parentWindow = qobject_cast<QWindow*>(d->visualParent)) {
        if (auto *parentQuickWindow = qobject_cast<QQuickWindow*>(parentWindow)) {
            parentItem = parentQuickWindow->contentItem();
        } else {
            qmlWarning(this) << "Parenting into non-Quick window. "
                << "Stacking, position, and destruction must be handled manually";
            QQuickWindow::setParent(parentWindow); // Try our best
            return;
        }
    }

    if (!parentItem) {
        qmlWarning(this) << "Unsupported visual parent type"
            << d->visualParent->metaObject()->className();
        return;
    }

    if (!parentItem->window()) {
        qCDebug(lcQuickWindow) << "No window yet. Deferring.";
        connect(parentItem, &QQuickItem::windowChanged, this, [this]{
            qCDebug(lcQuickWindow) << "Got window. Applying deferred visual parent item.";
            applyVisualParent();
        }, Qt::SingleShotConnection);
        return;
    }

    if (qobject_cast<QQuickWindowContainer*>(d->visualParent)) {
        qCDebug(lcQuickWindow) << "Visual parent is window container, everything is in order";
        return;
    }

    if (!d->windowContainer) {
        d->windowContainer = new QQuickWindowContainer(parentItem,
            QQuickWindowContainer::WindowControlsItem);
        d->windowContainer->setObjectName(objectName() + "Container"_L1);

        auto *objectParent = this->QObject::parent();
        if (objectParent == parentItem) {
            // We want to reflect the QML document order of sibling windows in the
            // resulting stacking order of the windows. We can do so by carefully
            // using the the information we have about the child object order.

            // We know that the window's object child index is correct in relation
            // to the other child windows of the parent. Since the window container
            // is going to represent the window from now on, make the window container
            // take the window's place in the parent's child object list.
            auto &objectChildren = QObjectPrivate::get(objectParent)->children;
            auto windowIndex = objectChildren.indexOf(this);
            auto containerIndex = objectChildren.indexOf(d->windowContainer);
            objectChildren.move(containerIndex, windowIndex);
            containerIndex = windowIndex;

            // The parent's item children are unfortunately managed separately from
            // the object children. But thanks to the logic above we can use the now
            // correct object order of the window container in the object children list
            // to also ensure a correct stacking order between the sibling child items.
            for (int i = containerIndex + 1; i < objectChildren.size(); ++i) {
                if (auto *childItem = qobject_cast<QQuickItem*>(objectChildren.at(i))) {
                    qCDebug(lcQuickWindow) << "Stacking" << d->windowContainer
                        << "below" << childItem;
                    d->windowContainer->stackBefore(childItem);
                    break;
                }
            }
        } else {
            // Having another visual parent than the direct object parent will
            // mess up the stacking order. This is also the case for normal items.
            qCDebug(lcQuickWindow) << "Visual parent is not object parent."
                << "Can not reflect document order as stacking order.";
        }

        QQmlEngine::setContextForObject(d->windowContainer, qmlContext(this));

        d->windowContainer->classBegin();
        d->windowContainer->setContainedWindow(this);
        // Once the window has a window container, all x/y/z changes of
        // the window will go through the container, and ensure the
        // correct mapping. But any changes that happened prior to
        // this have not been mapped yet, so do that now.
        d->windowContainer->setPosition(position());
        d->windowContainer->setZ(d->z);
        d->windowContainer->componentComplete();

        QObject::connect(d->windowContainer, &QQuickItem::zChanged,
            this, &QQuickWindowQmlImpl::zChanged);
    } else {
        d->windowContainer->setParentItem(parentItem);
    }
}

QObject *QQuickWindowQmlImpl::visualParent() const
{
    Q_D(const QQuickWindowQmlImpl);
    return d->visualParent;
}

// We shadow the x and y properties of the Window, so that in case
// the window has an Item as its visual parent we can re-map the
// coordinates via the corresponding window container. We need to
// do this also for the signal emissions, as otherwise the Window's
// change signals will reflect different values than what we report
// via the accessors. It would be nicer if this logic was contained
// in the window container, for example via meta object property
// interception, but that does not allow intercepting signal emissions.

void QQuickWindowQmlImpl::setX(int x)
{
    Q_D(QQuickWindowQmlImpl);
    if (Q_UNLIKELY(d->windowContainer && d->windowContainer->window()))
        d->windowContainer->setX(x);
    else
        QQuickWindow::setX(x);
}

int QQuickWindowQmlImpl::x() const
{
    Q_D(const QQuickWindowQmlImpl);
    if (Q_UNLIKELY(d->windowContainer && d->windowContainer->window()))
        return d->windowContainer->x();
    else
        return QQuickWindow::x();
}

void QQuickWindowQmlImpl::setY(int y)
{
    Q_D(QQuickWindowQmlImpl);
    if (Q_UNLIKELY(d->windowContainer && d->windowContainer->window()))
        d->windowContainer->setY(y);
    else
        QQuickWindow::setY(y);
}

int QQuickWindowQmlImpl::y() const
{
    Q_D(const QQuickWindowQmlImpl);
    if (Q_UNLIKELY(d->windowContainer && d->windowContainer->window()))
        return d->windowContainer->y();
    else
        return QQuickWindow::y();
}

/*!
    \qmlproperty real QtQuick::Window::z
    \internal

    Sets the stacking order of sibling windows.

    By default the stacking order is 0.

    Windows with a higher stacking value are drawn on top of windows with a
    lower stacking order. Windows with the same stacking value are drawn
    bottom up in the order they appear in the QML document.

    \note This property only has an effect for child windows.

    \sa QtQuick::Item::z
*/

void QQuickWindowQmlImpl::setZ(qreal z)
{
    Q_D(QQuickWindowQmlImpl);
    if (Q_UNLIKELY(d->windowContainer && d->windowContainer->window()))
        d->windowContainer->setZ(z);
    else
        d->z = z;
}

qreal QQuickWindowQmlImpl::z() const
{
    Q_D(const QQuickWindowQmlImpl);
    if (Q_UNLIKELY(d->windowContainer && d->windowContainer->window()))
        return d->windowContainer->z();
    else
        return d->z;
}

// --------------------------------------------------------------------

QObject *QQuickWindowQmlImpl::screen() const
{
    return new QQuickScreenInfo(const_cast<QQuickWindowQmlImpl *>(this), QWindow::screen());
}

void QQuickWindowQmlImpl::setScreen(QObject *screen)
{
    QQuickScreenInfo *screenWrapper = qobject_cast<QQuickScreenInfo *>(screen);
    QWindow::setScreen(screenWrapper ? screenWrapper->wrappedScreen() : nullptr);
}

QQuickWindowAttached *QQuickWindowQmlImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickWindowAttached(object);
}

QT_END_NAMESPACE

#include "moc_qquickwindowmodule_p.cpp"
