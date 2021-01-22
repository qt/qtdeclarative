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
