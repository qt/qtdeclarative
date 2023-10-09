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

Q_DECLARE_LOGGING_CATEGORY(lcTransient)

QQuickWindowQmlImplPrivate::QQuickWindowQmlImplPrivate() = default;

QQuickWindowQmlImpl::QQuickWindowQmlImpl(QWindow *parent)
    : QQuickWindowQmlImpl(*(new QQuickWindowQmlImplPrivate), parent)
{
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
    if (d->componentComplete)
        applyWindowVisibility();
}

QQuickWindowAttached *QQuickWindowQmlImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickWindowAttached(object);
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
        QV4::QObjectWrapper::wrap(v4, d->contentItem);
    }
}

void QQuickWindowQmlImpl::componentComplete()
{
    Q_D(QQuickWindowQmlImpl);
    qCDebug(lcQuickWindow) << "Component completed for" << this;
    d->componentComplete = true;

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

QQuickWindowQmlImpl::QQuickWindowQmlImpl(QQuickWindowQmlImplPrivate &dd, QWindow *parent)
    : QQuickWindow(dd, parent)
{
    // These two signals are called during QWindow's dtor, thus they have to be queued connections
    // or else our slots will be called instantly when our destructor has already run but our
    // connections haven't been removed yet.
    connect(this, &QWindow::visibleChanged, this, &QQuickWindowQmlImpl::visibleChanged,
            Qt::QueuedConnection);
    connect(this, &QWindow::visibilityChanged, this, &QQuickWindowQmlImpl::visibilityChanged,
            Qt::QueuedConnection);

    connect(this, &QWindow::screenChanged, this, &QQuickWindowQmlImpl::screenChanged);
}

bool QQuickWindowQmlImpl::event(QEvent *event)
{
    Q_D(QQuickWindowQmlImpl);

    if (event->type() == QEvent::ParentChange) {
        qCDebug(lcQuickWindow) << "Parent of" << this << "changed to" << QObject::parent();
        QObject::disconnect(d->itemParentWindowChangeListener);
        updateTransientParent();
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
    qCDebug(lcQuickWindow) << "Applying" << this << "visibility";

    const bool isAboutToShow = d->visibility == AutomaticVisibility
        ? d->visible : d->visibility != Hidden;

    if (isAboutToShow) {
        auto *itemParent = qmlobject_cast<QQuickItem *>(QObject::parent());
        if (!d->transientParentPropertySet && itemParent && !itemParent->window()) {
            qCDebug(lcTransient) << "Waiting for parent Item to resolve"
                                    "its transient parent. Deferring visibility";
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

    if (d->visibleExplicitlySet && ((d->visibility == Hidden && d->visible) ||
                                    (d->visibility > AutomaticVisibility && !d->visible))) {
        // FIXME: Should we bail out in this case?
        qmlWarning(this) << "Conflicting properties 'visible' and 'visibility'";
    }

    if (d->visibility == AutomaticVisibility) {
        setWindowState(QGuiApplicationPrivate::platformIntegration()->defaultWindowState(flags()));
        QQuickWindow::setVisible(d->visible);
    } else {
        QQuickWindow::setVisibility(d->visibility);
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

QObject *QQuickWindowQmlImpl::screen() const
{
    return new QQuickScreenInfo(const_cast<QQuickWindowQmlImpl *>(this), QWindow::screen());
}

void QQuickWindowQmlImpl::setScreen(QObject *screen)
{
    QQuickScreenInfo *screenWrapper = qobject_cast<QQuickScreenInfo *>(screen);
    QWindow::setScreen(screenWrapper ? screenWrapper->wrappedScreen() : nullptr);
}

QT_END_NAMESPACE

#include "moc_qquickwindowmodule_p.cpp"
