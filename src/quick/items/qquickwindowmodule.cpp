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
    if (d->componentComplete && (!transientParent() || transientParentVisible()))
        QQuickWindow::setVisible(visible);
}

void QQuickWindowQmlImpl::setVisibility(Visibility visibility)
{
    Q_D(QQuickWindowQmlImpl);
    d->visibility = visibility;
    if (d->componentComplete)
        QQuickWindow::setVisibility(visibility);
}

QQuickWindowAttached *QQuickWindowQmlImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickWindowAttached(object);
}

void QQuickWindowQmlImpl::classBegin()
{
    Q_D(QQuickWindowQmlImpl);
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
    d->componentComplete = true;

    QQuickItem *itemParent = qmlobject_cast<QQuickItem *>(QObject::parent());
    const bool transientParentAlreadySet = QQuickWindowPrivate::get(this)->transientParentPropertySet;
    if (!transientParentAlreadySet && itemParent && !itemParent->window()) {
        qCDebug(lcTransient) << "window" << title() << "has invisible Item parent" << itemParent << "transientParent"
                             << transientParent() << "declared visibility" << d->visibility << "; delaying show";
        connect(itemParent, &QQuickItem::windowChanged, this,
                &QQuickWindowQmlImpl::setWindowVisibility, Qt::QueuedConnection);
    } else if (transientParent() && !transientParent()->isVisible()) {
        connect(transientParent(), &QQuickWindow::visibleChanged, this,
                &QQuickWindowQmlImpl::setWindowVisibility, Qt::QueuedConnection);
    } else {
        setWindowVisibility();
    }
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

void QQuickWindowQmlImpl::setWindowVisibility()
{
    Q_D(QQuickWindowQmlImpl);
    if (transientParent() && !transientParentVisible())
        return;

    if (QQuickItem *senderItem = qmlobject_cast<QQuickItem *>(sender())) {
        disconnect(senderItem, &QQuickItem::windowChanged, this, &QQuickWindowQmlImpl::setWindowVisibility);
    } else if (sender()) {
        disconnect(transientParent(), &QWindow::visibleChanged, this, &QQuickWindowQmlImpl::setWindowVisibility);
    }

    // We have deferred window creation until we have the full picture of what
    // the user wanted in terms of window state, geometry, visibility, etc.

    if (d->visibleExplicitlySet && ((d->visibility == Hidden && d->visible) ||
                                    (d->visibility > AutomaticVisibility && !d->visible))) {
        QQmlData *data = QQmlData::get(this);
        Q_ASSERT(data && data->context);

        QQmlError error;
        error.setObject(this);

        QQmlRefPointer<QQmlContextData> urlContext = data->context;
        while (urlContext && urlContext->url().isEmpty())
            urlContext = urlContext->parent();
        error.setUrl(urlContext ? urlContext->url() : QUrl());

        QString objectId = data->context->findObjectId(this);
        if (!objectId.isEmpty())
            error.setDescription(QCoreApplication::translate("QQuickWindowQmlImpl",
                "Conflicting properties 'visible' and 'visibility' for Window '%1'").arg(objectId));
        else
            error.setDescription(QCoreApplication::translate("QQuickWindowQmlImpl",
                "Conflicting properties 'visible' and 'visibility'"));

        QQmlEnginePrivate::get(data->context->engine())->warning(error);
    }

    if (d->visibility == AutomaticVisibility) {
        setWindowState(QGuiApplicationPrivate::platformIntegration()->defaultWindowState(flags()));
        setVisible(d->visible);
    } else {
        setVisibility(d->visibility);
    }
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

QT_END_NAMESPACE

#include "moc_qquickwindowmodule_p.cpp"
