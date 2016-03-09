/****************************************************************************
**
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

#ifndef RENDERLOOP_H
#define RENDERLOOP_H

#include <private/qsgrenderloop_p.h>

QT_BEGIN_NAMESPACE

class RenderLoop : public QSGRenderLoop
{
    Q_OBJECT
public:
    RenderLoop();
    ~RenderLoop();

    void show(QQuickWindow *window) override;
    void hide(QQuickWindow *window) override;

    void windowDestroyed(QQuickWindow *window) override;

    void renderWindow(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window) override;
    QImage grab(QQuickWindow *window) override;

    void maybeUpdate(QQuickWindow *window) override;
    void update(QQuickWindow *window) override { maybeUpdate(window); } // identical for this implementation.
    void handleUpdateRequest(QQuickWindow *) override;

    void releaseResources(QQuickWindow *) override { }

    QSurface::SurfaceType windowSurfaceType() const override;

    QAnimationDriver *animationDriver() const override { return 0; }

    QSGContext *sceneGraphContext() const override;
    QSGRenderContext *createRenderContext(QSGContext *) const override { return rc; }

    struct WindowData {
        bool updatePending : 1;
        bool grabOnly : 1;
    };

    QHash<QQuickWindow *, WindowData> m_windows;

    QSGContext *sg;
    QSGRenderContext *rc;

    QImage grabContent;
};

QT_END_NAMESPACE

#endif // RENDERLOOP_H
