/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of Qt Quick 2d Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef THREADEDRENDERLOOP_H
#define THREADEDRENDERLOOP_H

#include <private/qsgrenderloop_p.h>

QT_BEGIN_NAMESPACE

class RenderThread;

class ThreadedRenderLoop : public QSGRenderLoop
{
    Q_OBJECT
public:
    ThreadedRenderLoop();

    void show(QQuickWindow *) {}
    void hide(QQuickWindow *);

    void windowDestroyed(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window);

    QImage grab(QQuickWindow *);

    void update(QQuickWindow *window);
    void maybeUpdate(QQuickWindow *window);
    QSGContext *sceneGraphContext() const;
    QSGRenderContext *createRenderContext(QSGContext *) const;

    QAnimationDriver *animationDriver() const;

    void releaseResources(QQuickWindow *window);

    bool event(QEvent *);

    bool interleaveIncubation() const;

public Q_SLOTS:
    void animationStarted();
    void animationStopped();

private:
    struct Window {
        QQuickWindow *window;
        RenderThread *thread;
        QSurfaceFormat actualWindowFormat;
        int timerId;
        uint updateDuringSync : 1;
    };

    friend class RenderThread;

    void releaseResources(Window *window, bool inDestructor);
    bool checkAndResetForceUpdate(QQuickWindow *window);
    Window *windowForTimer(int timerId) const;

    bool anyoneShowing() const;
    void initialize();

    void startOrStopAnimationTimer();
    void maybePostPolishRequest(Window *w);
    void waitForReleaseComplete();
    void polishAndSync(Window *w, bool inExpose = false);
    void maybeUpdate(Window *window);

    void handleExposure(QQuickWindow *w);
    void handleObscurity(Window *w);


    QSGContext *sg;
    QAnimationDriver *m_animation_driver;
    QList<Window> m_windows;

    int m_animation_timer;
    int m_exhaust_delay;

    bool m_lockedForSync;
};

QT_END_NAMESPACE

#endif // THREADEDRENDERLOOP_H
