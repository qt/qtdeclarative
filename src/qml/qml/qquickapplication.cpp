/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickapplication_p.h"
#include <private/qobject_p.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethod>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

class QQuickApplicationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickApplication)
public:
    QQuickApplicationPrivate() : active(QGuiApplication::activeWindow() != 0),
    layoutDirection(QGuiApplication::layoutDirection()) {}
    bool active;
    Qt::LayoutDirection layoutDirection;
};

/*
    This object and its properties are documented as part of the Qt object,
    in qdeclarativengine.cpp
*/

QQuickApplication::QQuickApplication(QObject *parent) : QObject(*new QQuickApplicationPrivate(), parent)
{
    if (qApp)
        qApp->installEventFilter(this);
}

QQuickApplication::~QQuickApplication()
{
}

bool QQuickApplication::active() const
{
    Q_D(const QQuickApplication);
    return d->active;
}

Qt::LayoutDirection QQuickApplication::layoutDirection() const
{
    Q_D(const QQuickApplication);
    return d->layoutDirection;
}

QObject *QQuickApplication::inputPanel() const
{
    static bool warned = false;
    if (!warned) {
        qWarning() << "Qt.application.inputPanel is deprecated, use Qt.inputMethod instead";
        warned = true;
    }
    return qApp ? qApp->inputMethod() : 0;
}

bool QQuickApplication::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
    Q_D(QQuickApplication);
    if (event->type() == QEvent::ApplicationActivate
     || event->type() == QEvent::ApplicationDeactivate) {
        bool active = d->active;
        if (event->type() == QEvent::ApplicationActivate)
            active  = true;
        else if (event->type() == QEvent::ApplicationDeactivate)
            active  = false;

        if (d->active != active) {
            d->active = active;
            emit activeChanged();
        }
    }
    if (event->type() == QEvent::LayoutDirectionChange) {
        Qt::LayoutDirection direction = QGuiApplication::layoutDirection();
        if (d->layoutDirection != direction) {
            d->layoutDirection = direction;
            emit layoutDirectionChanged();
        }
    }
    return false;
}

QT_END_NAMESPACE
