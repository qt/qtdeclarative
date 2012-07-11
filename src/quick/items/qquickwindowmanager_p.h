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

#ifndef QQUICKWINDOWMANAGER_P_H
#define QQUICKWINDOWMANAGER_P_H

#include <QtGui/QImage>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QSGContext;

class Q_QUICK_PRIVATE_EXPORT QQuickWindowManager
{
public:
    virtual ~QQuickWindowManager();

    virtual void show(QQuickWindow *window) = 0;
    virtual void hide(QQuickWindow *window) = 0;

    virtual void windowDestroyed(QQuickWindow *window) = 0;

    virtual void exposureChanged(QQuickWindow *window) = 0;
    virtual QImage grab(QQuickWindow *window) = 0;
    virtual void resize(QQuickWindow *window, const QSize &size) = 0;

    virtual void update(QQuickWindow *window) = 0;
    virtual void maybeUpdate(QQuickWindow *window) = 0;
    virtual void wakeup() = 0;

    virtual volatile bool *allowMainThreadProcessing() = 0;

    virtual QSGContext *sceneGraphContext() const = 0;

    virtual void releaseResources() = 0;

    // ### make this less of a singleton
    static QQuickWindowManager *instance();
};

QT_END_NAMESPACE

#endif // QQUICKWINDOWMANAGER_P_H
