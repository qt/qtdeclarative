/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSGTHREADEDRENDERLOOP_P_H
#define QSGTHREADEDRENDERLOOP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QThread>
#include <QtGui/QOpenGLContext>
#include <private/qsgcontext_p.h>

#include "qsgrenderloop_p.h"

QT_BEGIN_NAMESPACE

class QSGRenderThread;

class QSGThreadedRenderLoop : public QSGRenderLoop
{
    Q_OBJECT
public:
    QSGThreadedRenderLoop();
    ~QSGThreadedRenderLoop();

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
        QSGRenderThread *thread;
        QSurfaceFormat actualWindowFormat;
        uint updateDuringSync : 1;
        uint forceRenderPass : 1;
    };

    friend class QSGRenderThread;

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
    void releaseSwapchain(QQuickWindow *window);

    bool eventFilter(QObject *watched, QEvent *event) override;

    QSGContext *sg;
    // Set of contexts that have been created but are now owned by
    // a rendering thread yet, as the window has never been exposed.
    mutable QSet<QSGRenderContext*> pendingRenderContexts;
    QAnimationDriver *m_animation_driver;
    QList<Window> m_windows;

    int m_animation_timer;

    bool m_lockedForSync;
};



QT_END_NAMESPACE

#endif // QSGTHREADEDRENDERLOOP_P_H
