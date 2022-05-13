// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_SINGLETHREADED_H
#define WINDOW_SINGLETHREADED_H

#include <QWindow>
#include <QMatrix4x4>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QOpenGLContext;
class QOpenGLTexture;
class QOffscreenSurface;
class QQuickRenderControl;
class QQuickWindow;
class QQmlEngine;
class QQmlComponent;
class QQuickItem;
QT_END_NAMESPACE

class CubeRenderer;

class WindowSingleThreaded : public QWindow
{
    Q_OBJECT

public:
    WindowSingleThreaded();
    ~WindowSingleThreaded();

protected:
    void exposeEvent(QExposeEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

private slots:
    void run();

    void createTexture();
    void destroyTexture();
    void render();
    void requestUpdate();
    void handleScreenChange();

private:
    void startQuick(const QString &filename);
    void updateSizes();
    void resizeTexture();

    QOpenGLContext *m_context;
    QOffscreenSurface *m_offscreenSurface;
    QQuickRenderControl *m_renderControl;
    QQuickWindow *m_quickWindow;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    QQuickItem *m_rootItem;
    uint m_textureId;
    QSize m_textureSize;
    bool m_quickInitialized;
    bool m_quickReady;
    QTimer m_updateTimer;
    CubeRenderer *m_cubeRenderer;
    qreal m_dpr;
};

#endif
