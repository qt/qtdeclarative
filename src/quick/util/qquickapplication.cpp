/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickapplication_p.h"

#include <private/qobject_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QtGui/QGuiApplication>
#include <QtCore/QDebug>
#include <QtQml/private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickApplicationPrivate : public QQmlApplicationPrivate
{
    Q_DECLARE_PUBLIC(QQuickApplication)
public:
    QQuickApplicationPrivate()
        : isActive(QGuiApplication::focusWindow() != 0),
          direction(QGuiApplication::layoutDirection())
    {
    }

private:
    bool isActive;
    Qt::LayoutDirection direction;
};

/*
    This object and its properties are documented as part of the Qt object,
    in qqmlengine.cpp
*/

QQuickApplication::QQuickApplication(QObject *parent)
    : QQmlApplication(*new QQuickApplicationPrivate(), parent)
{
    if (qApp) {
        qApp->installEventFilter(this);

        connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)),
                this, SIGNAL(stateChanged(Qt::ApplicationState)));
    }
}

QQuickApplication::~QQuickApplication()
{
}

bool QQuickApplication::active() const
{
    Q_D(const QQuickApplication);
    return d->isActive;
}

Qt::LayoutDirection QQuickApplication::layoutDirection() const
{
    Q_D(const QQuickApplication);
    return d->direction;
}

bool QQuickApplication::supportsMultipleWindows() const
{
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows);
}

Qt::ApplicationState QQuickApplication::state() const
{
    return QGuiApplication::applicationState();
}

bool QQuickApplication::eventFilter(QObject *, QEvent *event)
{
    Q_D(QQuickApplication);
    if ((event->type() == QEvent::ApplicationActivate) ||
        (event->type() == QEvent::ApplicationDeactivate) ||
        (event->type() == QEvent::ApplicationStateChange)) {
        bool wasActive = d->isActive;
        if (event->type() == QEvent::ApplicationStateChange) {
            QApplicationStateChangeEvent * e= static_cast<QApplicationStateChangeEvent*>(event);
            d->isActive = e->applicationState() == Qt::ApplicationActive;
        } else {
            d->isActive = (event->type() == QEvent::ApplicationActivate);
        }
        if (d->isActive != wasActive) {
            emit activeChanged();
        }
    } else if (event->type() == QEvent::ApplicationLayoutDirectionChange) {
        Qt::LayoutDirection newDirection = QGuiApplication::layoutDirection();
        if (d->direction != newDirection) {
            d->direction = newDirection;
            emit layoutDirectionChanged();
        }
    }
    return false;
}

QT_END_NAMESPACE
