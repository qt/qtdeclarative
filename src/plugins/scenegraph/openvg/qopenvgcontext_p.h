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
