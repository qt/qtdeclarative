// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_MULTITHREADED_H
#define WINDOW_MULTITHREADED_H

#include <QWindow>
#include <QMatrix4x4>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
QT_FORWARD_DECLARE_CLASS(QOffscreenSurface)
QT_FORWARD_DECLARE_CLASS(QQuickRenderControl)
QT_FORWARD_DECLARE_CLASS(QQuickWindow)
QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQmlComponent)
QT_FORWARD_DECLARE_CLASS(QQuickItem)
QT_FORWARD_DECLARE_CLASS(QRhi)

class CubeRenderer;

class QuickRenderer : public QObject
{
    Q_OBJECT

public:
    QuickRenderer();

    void requestInit();
    void requestRender();
    void requestResize();
    void requestCleanup();
    void requestStop();

    QWaitCondition *cond() { return &m_cond; }
    QMutex *mutex() { return &m_mutex; }

    void setContext(QOpenGLContext *ctx) { m_context = ctx; }
    void setSurface(QOffscreenSurface *s) { m_surface = s; }
    void setWindow(QWindow *w) { m_window = w; }
    void setQuickWindow(QQuickWindow *w) { m_quickWindow = w; }
    void setRenderControl(QQuickRenderControl *r) { m_renderControl = r; }

    void aboutToQuit();

private:
    bool event(QEvent *e) override;
    void init();
    void cleanup();
    void cleanupRhi();
    void ensureTexture();
    void render(QMutexLocker<QMutex> *lock);

    QWaitCondition m_cond;
    QMutex m_mutex;
    QOpenGLContext *m_context;
    QOffscreenSurface *m_surface;
    QWindow *m_window;
    QQuickWindow *m_quickWindow;
    QQuickRenderControl *m_renderControl;
    QRhi *m_rhi;
    uint m_textureId;
    CubeRenderer *m_cubeRenderer;
    QMutex m_quitMutex;
    bool m_quit;
};

class WindowMultiThreaded : public QWindow
{
    Q_OBJECT

public:
    WindowMultiThreaded();
    ~WindowMultiThreaded();

protected:
    void exposeEvent(QExposeEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool event(QEvent *e) override;

private slots:
    void run();
    void requestUpdate();
    void polishSyncAndRender();

private:
    void startQuick(const QString &filename);
    void updateSizes();

    QuickRenderer *m_quickRenderer;
    QThread *m_quickRendererThread;

    QOpenGLContext *m_context;
    QOffscreenSurface *m_offscreenSurface;
    QQuickRenderControl *m_renderControl;
    QQuickWindow *m_quickWindow;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    QQuickItem *m_rootItem;
    bool m_quickInitialized;
    bool m_psrRequested;
};

#endif
