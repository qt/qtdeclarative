// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENVGOFFSCREENSURFACE_H
#define QOPENVGOFFSCREENSURFACE_H

#include "qopenvgcontext_p.h"

QT_BEGIN_NAMESPACE

class QOpenVGOffscreenSurface
{
public:
    QOpenVGOffscreenSurface(const QSize &size);
    ~QOpenVGOffscreenSurface();

    void makeCurrent();
    void doneCurrent();
    void swapBuffers();

    VGImage image() { return m_image; }
    QSize size() const { return m_size; }

    QImage readbackQImage();

private:
    VGImage m_image;
    QSize m_size;
    EGLContext m_context;
    EGLSurface m_renderTarget;
    EGLContext m_previousContext = EGL_NO_CONTEXT;
    EGLSurface m_previousReadSurface = EGL_NO_SURFACE;
    EGLSurface m_previousDrawSurface = EGL_NO_SURFACE;
    EGLDisplay m_display;
};

QT_END_NAMESPACE

#endif // QOPENVGOFFSCREENSURFACE_H
