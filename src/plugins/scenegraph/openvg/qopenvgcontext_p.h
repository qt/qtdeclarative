// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENVGCONTEXT_H
#define QOPENVGCONTEXT_H

#include <QtGui/QWindow>
#include <QtGui/QImage>

#include <EGL/egl.h>
#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

class QOpenVGContext
{
public:
    QOpenVGContext(QWindow *window);
    ~QOpenVGContext();

    void makeCurrent();
    void makeCurrent(EGLSurface surface);
    void doneCurrent();
    void swapBuffers();
    void swapBuffers(EGLSurface surface);


    QWindow *window() const;

    EGLDisplay eglDisplay() { return m_display; }
    EGLConfig eglConfig() { return m_config; }
    EGLContext eglContext() { return m_context; }

    QImage readFramebuffer(const QSize &size);

    void getConfigs();

    static void checkErrors();

private:
    EGLSurface m_surface;
    EGLDisplay m_display;
    EGLConfig m_config;
    EGLContext m_context;

    QWindow *m_window;

};

QT_END_NAMESPACE

#endif // QOPENVGCONTEXT_H
