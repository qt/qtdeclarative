// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGRENDERLOOP_H
#define QSGOPENVGRENDERLOOP_H

#include <private/qsgrenderloop_p.h>

QT_BEGIN_NAMESPACE

class QOpenVGContext;

class QSGOpenVGRenderLoop : public QSGRenderLoop
{
public:
    QSGOpenVGRenderLoop();
    ~QSGOpenVGRenderLoop();


    void show(QQuickWindow *window) override;
    void hide(QQuickWindow *window) override;

    void windowDestroyed(QQuickWindow *window) override;

    void renderWindow(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window) override;
    QImage grab(QQuickWindow *window) override;

    void maybeUpdate(QQuickWindow *window) override;
    void update(QQuickWindow *window) override;
    void handleUpdateRequest(QQuickWindow *window) override;

    void releaseResources(QQuickWindow *) override;

    QSurface::SurfaceType windowSurfaceType() const override;

    QAnimationDriver *animationDriver() const override;

    QSGContext *sceneGraphContext() const override;
    QSGRenderContext *createRenderContext(QSGContext *) const override;

    struct WindowData {
        bool updatePending : 1;
        bool grabOnly : 1;
    };

    QHash<QQuickWindow *, WindowData> m_windows;

    QSGContext *sg;
    QSGRenderContext *rc;
    QOpenVGContext *vg;

    QImage grabContent;
};

QT_END_NAMESPACE

#endif // QSGOPENVGRENDERLOOP_H
