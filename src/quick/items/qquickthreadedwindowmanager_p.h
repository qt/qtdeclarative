/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKTHREADEDWINDOWMANAGER_P_H
#define QQUICKTHREADEDWINDOWMANAGER_P_H

#include "qquickwindowmanager_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/private/qabstractanimation_p.h>

#include <QtGui/QOpenGLContext>
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickRenderThreadSingleContextWindowManager : public QThread, public QQuickWindowManager
{
    Q_OBJECT
public:
    QQuickRenderThreadSingleContextWindowManager()
        : sg(QSGContext::createDefaultContext())
        , gl(0)
        , animationTimer(-1)
        , isGuiLocked(0)
        , animationRunning(false)
        , isPostingSyncEvent(false)
        , isRenderBlocked(false)
        , isExternalUpdatePending(false)
        , syncAlreadyHappened(false)
        , inSync(false)
        , shouldExit(false)
        , hasExited(false)
        , isDeferredUpdatePosted(false)
        , windowToGrab(0)
    {
        sg->moveToThread(this);

        animDriver = sg->createAnimationDriver(this);
        animDriver->install();
        connect(animDriver, SIGNAL(started()), this, SLOT(animationStarted()));
        connect(animDriver, SIGNAL(stopped()), this, SLOT(animationStopped()));
    }

    QSGContext *sceneGraphContext() const { return sg; }

    void releaseResources() { }

    void show(QQuickWindow *window);
    void hide(QQuickWindow *window);

    void windowDestroyed(QQuickWindow *window);

    void exposureChanged(QQuickWindow *window);
    QImage grab(QQuickWindow *window);
    void resize(QQuickWindow *window, const QSize &size);
    void handleDeferredUpdate();
    void maybeUpdate(QQuickWindow *window);
    void update(QQuickWindow *window) { maybeUpdate(window); } // identical for this implementation

    void startRendering();
    void stopRendering();

    void exhaustSyncEvent();
    void sync(bool guiAlreadyLocked);

    void initialize();

    bool event(QEvent *);

    inline void lock() { mutex.lock(); }
    inline void unlock() { mutex.unlock(); }
    inline void wait() { condition.wait(&mutex); }
    inline void wake() { condition.wakeOne(); }
    void lockInGui();
    void unlockInGui();

    void run();

    QAnimationDriver *animationDriver() const { return animDriver; }

public slots:
    void animationStarted();
    void animationStopped();
    void windowVisibilityChanged();

private:
    void handleAddedWindows();
    void handleAddedWindow(QQuickWindow *window);
    void handleRemovedWindows(bool clearGLContext = true);

    QSGContext *sg;
    QOpenGLContext *gl;
    QAnimationDriver *animDriver;
    int animationTimer;

    QMutex mutex;
    QWaitCondition condition;

    int isGuiLocked;
    uint animationRunning: 1;
    uint isPostingSyncEvent : 1;
    uint isRenderBlocked : 1;
    uint isExternalUpdatePending : 1;
    uint syncAlreadyHappened : 1;
    uint inSync : 1;
    uint shouldExit : 1;
    uint hasExited : 1;
    uint isDeferredUpdatePosted : 1;

    QQuickWindow *windowToGrab;
    QImage grabContent;

    struct WindowData {
        QSize renderedSize;
        QSize windowSize;
        QSize viewportSize;

        uint sizeWasChanged : 1;
        uint isVisible : 1;
        uint isRenderable : 1;
    };

    QHash<QQuickWindow *, WindowData *> m_rendered_windows;

    struct WindowTracker {
        QQuickWindow *window;
        uint isVisible : 1;
        uint toBeRemoved : 1;
    };

    QList<WindowTracker> m_tracked_windows;

    QList<QQuickWindow *> m_removed_windows;
    QList<QQuickWindow *> m_added_windows;
};

QT_END_NAMESPACE

#endif // QQUICKTHREADEDWINDOWMANAGER_P_H
