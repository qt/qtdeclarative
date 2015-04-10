/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickwindowmodule_p.h"
#include "qquickwindowattached_p.h"
#include "qquickscreen_p.h"
#include "qquickview_p.h"
#include <QtQuick/QQuickWindow>
#include <QtCore/QCoreApplication>
#include <QtQml/QQmlEngine>

#include <private/qguiapplication_p.h>
#include <private/qqmlengine_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

class QQuickWindowQmlImplPrivate : public QQuickWindowPrivate
{
public:
    QQuickWindowQmlImplPrivate()
        : complete(false)
        , visible(false)
        , visibility(QQuickWindow::AutomaticVisibility)
    {
    }

    bool complete;
    bool visible;
    QQuickWindow::Visibility visibility;
    QV4::PersistentValue rootItemMarker;
};

QQuickWindowQmlImpl::QQuickWindowQmlImpl(QWindow *parent)
    : QQuickWindow(*(new QQuickWindowQmlImplPrivate), parent)
{
    connect(this, &QWindow::visibleChanged, this, &QQuickWindowQmlImpl::visibleChanged);
    connect(this, &QWindow::visibilityChanged, this, &QQuickWindowQmlImpl::visibilityChanged);
}

void QQuickWindowQmlImpl::setVisible(bool visible)
{
    Q_D(QQuickWindowQmlImpl);
    if (!d->complete)
        d->visible = visible;
    else if (!transientParent() || transientParent()->isVisible())
        QQuickWindow::setVisible(visible);
}

void QQuickWindowQmlImpl::setVisibility(Visibility visibility)
{
    Q_D(QQuickWindowQmlImpl);
    if (!d->complete)
        d->visibility = visibility;
    else
        QQuickWindow::setVisibility(visibility);
}

QQuickWindowAttached *QQuickWindowQmlImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickWindowAttached(object);
}

void QQuickWindowQmlImpl::classBegin()
{
    Q_D(QQuickWindowQmlImpl);
    QQmlEngine* e = qmlEngine(this);
    //Give QQuickView behavior when created from QML with QQmlApplicationEngine
    if (QCoreApplication::instance()->property("__qml_using_qqmlapplicationengine") == QVariant(true)) {
        if (e && !e->incubationController())
            e->setIncubationController(incubationController());
    }
    Q_ASSERT(e);
    {
        QV4::ExecutionEngine *v4 = QQmlEnginePrivate::getV4Engine(e);
        QV4::Scope scope(v4);
        QV4::ScopedObject v(scope, QV4::QQuickRootItemMarker::create(e, this));
        d->rootItemMarker = v;
    }
}

void QQuickWindowQmlImpl::componentComplete()
{
    Q_D(QQuickWindowQmlImpl);
    d->complete = true;
    if (transientParent() && !transientParent()->isVisible()) {
        connect(transientParent(), &QQuickWindow::visibleChanged, this,
                &QQuickWindowQmlImpl::setWindowVisibility, Qt::QueuedConnection);
    } else {
        setWindowVisibility();
    }
}

void QQuickWindowQmlImpl::setWindowVisibility()
{
    Q_D(QQuickWindowQmlImpl);
    if (transientParent() && !transientParent()->isVisible())
        return;

    if (sender()) {
        disconnect(transientParent(), &QWindow::visibleChanged, this,
                   &QQuickWindowQmlImpl::setWindowVisibility);
    }

    // We have deferred window creation until we have the full picture of what
    // the user wanted in terms of window state, geometry, visibility, etc.

    if ((d->visibility == Hidden && d->visible) || (d->visibility > AutomaticVisibility && !d->visible)) {
        QQmlData *data = QQmlData::get(this);
        Q_ASSERT(data && data->context);

        QQmlError error;
        error.setObject(this);

        const QQmlContextData* urlContext = data->context;
        while (urlContext && urlContext->url().isEmpty())
            urlContext = urlContext->parent;
        error.setUrl(urlContext ? urlContext->url() : QUrl());

        QString objectId = data->context->findObjectId(this);
        if (!objectId.isEmpty())
            error.setDescription(QCoreApplication::translate("QQuickWindowQmlImpl",
                "Conflicting properties 'visible' and 'visibility' for Window '%1'").arg(objectId));
        else
            error.setDescription(QCoreApplication::translate("QQuickWindowQmlImpl",
                "Conflicting properties 'visible' and 'visibility'"));

        QQmlEnginePrivate::get(data->context->engine)->warning(error);
    }

    if (d->visibility == AutomaticVisibility) {
        setWindowState(QGuiApplicationPrivate::platformIntegration()->defaultWindowState(flags()));
        setVisible(d->visible);
    } else {
        setVisibility(d->visibility);
    }
}

void QQuickWindowModule::defineModule()
{
    const char uri[] = "QtQuick.Window";

    qmlRegisterType<QQuickWindow>(uri, 2, 0, "Window");
    qmlRegisterRevision<QWindow,1>(uri, 2, 1);
    qmlRegisterRevision<QWindow,2>(uri, 2, 2);
    qmlRegisterRevision<QQuickWindow,1>(uri, 2, 1);//Type moved to a subclass, but also has new members
    qmlRegisterRevision<QQuickWindow,2>(uri, 2, 2);
    qmlRegisterType<QQuickWindowQmlImpl>(uri, 2, 1, "Window");
    qmlRegisterType<QQuickWindowQmlImpl,1>(uri, 2, 2, "Window");
    qmlRegisterUncreatableType<QQuickScreen>(uri, 2, 0, "Screen", QStringLiteral("Screen can only be used via the attached property."));
}

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickWindowQmlImpl, QML_HAS_ATTACHED_PROPERTIES)
