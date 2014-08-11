/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Purchasing Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef RENDERLOOP_H
#define RENDERLOOP_H

#include <private/qsgrenderloop_p.h>

class RenderLoop : public QSGRenderLoop
{
    Q_OBJECT
public:
    RenderLoop();
    ~RenderLoop();

    void show(QQuickWindow *window);
    void hide(QQuickWindow *window);

    void windowDestroyed(QQuickWindow *window);

    void renderWindow(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window);
    QImage grab(QQuickWindow *window);

    void maybeUpdate(QQuickWindow *window);
    void update(QQuickWindow *window) { maybeUpdate(window); } // identical for this implementation.

    void releaseResources(QQuickWindow *) { }

    virtual QSurface::SurfaceType windowSurfaceType() const;

    QAnimationDriver *animationDriver() const { return 0; }

    QSGContext *sceneGraphContext() const;
    QSGRenderContext *createRenderContext(QSGContext *) const { return rc; }

    bool event(QEvent *);

    struct WindowData {
        bool updatePending : 1;
        bool grabOnly : 1;
    };

    QHash<QQuickWindow *, WindowData> m_windows;

    QSGContext *sg;
    QSGRenderContext *rc;

    QImage grabContent;
    int m_update_timer;

    bool eventPending;

};

#endif // RENDERLOOP_H
