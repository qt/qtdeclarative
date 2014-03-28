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

#ifndef QSGRenderLoop_P_H
#define QSGRenderLoop_P_H

#include <QtGui/QImage>
#include <private/qtquickglobal_p.h>
#include <QtCore/QSet>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QSGContext;
class QSGRenderContext;
class QAnimationDriver;

class Q_QUICK_PRIVATE_EXPORT QSGRenderLoop : public QObject
{
    Q_OBJECT

public:
    virtual ~QSGRenderLoop();

    virtual void show(QQuickWindow *window) = 0;
    virtual void hide(QQuickWindow *window) = 0;
    virtual void resize(QQuickWindow *) {};

    virtual void windowDestroyed(QQuickWindow *window) = 0;

    virtual void exposureChanged(QQuickWindow *window) = 0;
    virtual QImage grab(QQuickWindow *window) = 0;

    virtual void update(QQuickWindow *window) = 0;
    virtual void maybeUpdate(QQuickWindow *window) = 0;

    virtual QAnimationDriver *animationDriver() const = 0;

    virtual QSGContext *sceneGraphContext() const = 0;
    virtual QSGRenderContext *createRenderContext(QSGContext *) const = 0;

    virtual void releaseResources(QQuickWindow *window) = 0;

    void addWindow(QQuickWindow *win) { m_windows.insert(win); }
    void removeWindow(QQuickWindow *win) { m_windows.remove(win); }
    QSet<QQuickWindow *> windows() const { return m_windows; }

    // ### make this less of a singleton
    static QSGRenderLoop *instance();
    static void setInstance(QSGRenderLoop *instance);

    static bool useConsistentTiming();

    virtual bool interleaveIncubation() const { return false; }

    static void cleanup();

Q_SIGNALS:
    void timeToIncubate();

protected:
    void handleContextCreationFailure(QQuickWindow *window, bool isEs);

private:
    static QSGRenderLoop *s_instance;

    QSet<QQuickWindow *> m_windows;
};

QT_END_NAMESPACE

#endif // QSGRenderLoop_P_H
