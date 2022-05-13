// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRENDERLOOP_P_H
#define QSGRENDERLOOP_P_H

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

#include <QtGui/qimage.h>
#include <QtGui/qsurface.h>
#include <private/qtquickglobal_p.h>
#include <QtCore/qset.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QSGContext;
class QSGRenderContext;
class QAnimationDriver;
class QRunnable;

class Q_QUICK_PRIVATE_EXPORT QSGRenderLoop : public QObject
{
    Q_OBJECT

public:
    enum RenderLoopFlags {
        SupportsGrabWithoutExpose = 0x01
    };

    virtual ~QSGRenderLoop();

    virtual void show(QQuickWindow *window) = 0;
    virtual void hide(QQuickWindow *window) = 0;
    virtual void resize(QQuickWindow *) {};

    virtual void windowDestroyed(QQuickWindow *window) = 0;

    virtual void exposureChanged(QQuickWindow *window) = 0;
    virtual QImage grab(QQuickWindow *window) = 0;

    virtual void update(QQuickWindow *window) = 0;
    virtual void maybeUpdate(QQuickWindow *window) = 0;
    virtual void handleUpdateRequest(QQuickWindow *) { }

    virtual QAnimationDriver *animationDriver() const = 0;

    virtual QSGContext *sceneGraphContext() const = 0;
    virtual QSGRenderContext *createRenderContext(QSGContext *) const = 0;

    virtual void releaseResources(QQuickWindow *window) = 0;
    virtual void postJob(QQuickWindow *window, QRunnable *job);

    void addWindow(QQuickWindow *win) { m_windows.insert(win); }
    void removeWindow(QQuickWindow *win) { m_windows.remove(win); }
    QSet<QQuickWindow *> windows() const { return m_windows; }

    virtual QSurface::SurfaceType windowSurfaceType() const;

    // ### make this less of a singleton
    static QSGRenderLoop *instance();
    static void setInstance(QSGRenderLoop *instance);

    virtual bool interleaveIncubation() const { return false; }

    virtual int flags() const { return 0; }

    static void cleanup();

    void handleContextCreationFailure(QQuickWindow *window);

Q_SIGNALS:
    void timeToIncubate();

private:
    static QSGRenderLoop *s_instance;

    QSet<QQuickWindow *> m_windows;
};

enum QSGRenderLoopType
{
    BasicRenderLoop,
    ThreadedRenderLoop
};

QT_END_NAMESPACE

#endif // QSGRENDERLOOP_P_H
