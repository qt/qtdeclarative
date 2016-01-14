/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2D Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
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

    void show(QQuickWindow *) override {}
    void hide(QQuickWindow *) override;

    void windowDestroyed(QQuickWindow *window) override;
    void exposureChanged(QQuickWindow *window) override;

    QImage grab(QQuickWindow *) override;

    void update(QQuickWindow *window) override;
    void maybeUpdate(QQuickWindow *window) override;
    void handleUpdateRequest(QQuickWindow *window) override;

    QSGContext *sceneGraphContext() const override;
    QSGRenderContext *createRenderContext(QSGContext *) const override;

    QAnimationDriver *animationDriver() const override;

    void releaseResources(QQuickWindow *window) override;

    bool event(QEvent *) override;
    void postJob(QQuickWindow *window, QRunnable *job) override;

    bool interleaveIncubation() const override;

public Q_SLOTS:
    void animationStarted();
    void animationStopped();

private:
    struct Window {
        QQuickWindow *window;
        RenderThread *thread;
        QSurfaceFormat actualWindowFormat;
        uint updateDuringSync : 1;
        uint forceRenderPass : 1;
    };

    friend class RenderThread;

    void releaseResources(Window *window, bool inDestructor);
    bool checkAndResetForceUpdate(QQuickWindow *window);

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

    bool m_lockedForSync;
};

QT_END_NAMESPACE

#endif // THREADEDRENDERLOOP_H
