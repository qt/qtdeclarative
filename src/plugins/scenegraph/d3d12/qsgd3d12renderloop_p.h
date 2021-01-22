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

#ifndef QSGD3D12RENDERLOOP_P_H
#define QSGD3D12RENDERLOOP_P_H

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

#include <private/qsgrenderloop_p.h>

QT_BEGIN_NAMESPACE

class QSGD3D12Engine;
class QSGD3D12Context;
class QSGD3D12RenderContext;

class QSGD3D12RenderLoop : public QSGRenderLoop
{
    Q_OBJECT

public:
    QSGD3D12RenderLoop();
    ~QSGD3D12RenderLoop();

    void show(QQuickWindow *window) override;
    void hide(QQuickWindow *window) override;
    void resize(QQuickWindow *window) override;

    void windowDestroyed(QQuickWindow *window) override;

    void exposureChanged(QQuickWindow *window) override;

    QImage grab(QQuickWindow *window) override;

    void update(QQuickWindow *window) override;
    void maybeUpdate(QQuickWindow *window) override;

    QAnimationDriver *animationDriver() const override;

    QSGContext *sceneGraphContext() const override;
    QSGRenderContext *createRenderContext(QSGContext *) const override;

    void releaseResources(QQuickWindow *window) override;
    void postJob(QQuickWindow *window, QRunnable *job) override;

    QSurface::SurfaceType windowSurfaceType() const override;
    bool interleaveIncubation() const override;
    int flags() const override;

    bool event(QEvent *event) override;

public Q_SLOTS:
    void onAnimationStarted();
    void onAnimationStopped();

private:
    void exposeWindow(QQuickWindow *window);
    void obscureWindow(QQuickWindow *window);
    void renderWindow(QQuickWindow *window);
    void render();
    void maybePostUpdateTimer();
    bool somethingVisible() const;

    QSGD3D12Context *sg;
    QAnimationDriver *m_anims;
    int m_vsyncDelta;
    int m_updateTimer = 0;
    int m_animationTimer = 0;

    struct WindowData {
        QSGD3D12RenderContext *rc = nullptr;
        QSGD3D12Engine *engine = nullptr;
        bool updatePending = false;
        bool grabOnly = false;
        bool exposed = false;
    };

    QHash<QQuickWindow *, WindowData> m_windows;

    QImage m_grabContent;
};

QT_END_NAMESPACE

#endif // QSGD3D12RENDERLOOP_P_H
