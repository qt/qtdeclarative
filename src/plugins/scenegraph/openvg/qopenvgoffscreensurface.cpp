// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>
#include "qopenvgoffscreensurface.h"

#include <QtGui/QImage>


QT_BEGIN_NAMESPACE

QOpenVGOffscreenSurface::QOpenVGOffscreenSurface(const QSize &size)
    : m_size(size)
{
    m_display = eglGetCurrentDisplay();
    m_image = vgCreateImage(VG_sARGB_8888_PRE, m_size.width(), m_size.height(), VG_IMAGE_QUALITY_BETTER);

    const EGLint configAttribs[] = {
        EGL_CONFORMANT, EGL_OPENVG_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_ALPHA_MASK_SIZE, 8,
        EGL_NONE
    };

    EGLConfig pbufferConfig;
    EGLint numConfig;
    eglChooseConfig(m_display, configAttribs, &pbufferConfig, 1, &numConfig);

    m_context = eglCreateContext(m_display, pbufferConfig, eglGetCurrentContext(), 0);
    if (m_context == EGL_NO_CONTEXT)
        qWarning("QOpenVGOffscreenSurface: failed to create EGLContext");

    m_renderTarget = eglCreatePbufferFromClientBuffer(m_display,
                                                      EGL_OPENVG_IMAGE,
                                                      reinterpret_cast<EGLClientBuffer>(uintptr_t(m_image)),
                                                      pbufferConfig,
                                                      0);
    if (m_renderTarget == EGL_NO_SURFACE)
        qWarning("QOpenVGOffscreenSurface: failed to create EGLSurface from VGImage");
}

QOpenVGOffscreenSurface::~QOpenVGOffscreenSurface()
{
    vgDestroyImage(m_image);
    eglDestroySurface(m_display, m_renderTarget);
    eglDestroyContext(m_display, m_context);
}

void QOpenVGOffscreenSurface::makeCurrent()
{
    EGLContext currentContext = eglGetCurrentContext();
    if (m_context != currentContext) {
        m_previousContext = eglGetCurrentContext();
        m_previousReadSurface = eglGetCurrentSurface(EGL_READ);
        m_previousDrawSurface = eglGetCurrentSurface(EGL_DRAW);

        eglMakeCurrent(m_display, m_renderTarget, m_renderTarget, m_context);
    }
}

void QOpenVGOffscreenSurface::doneCurrent()
{
    EGLContext currentContext = eglGetCurrentContext();
    if (m_context == currentContext) {
        eglMakeCurrent(m_display, m_previousDrawSurface, m_previousReadSurface, m_previousContext);
        m_previousContext = EGL_NO_CONTEXT;
        m_previousReadSurface = EGL_NO_SURFACE;
        m_previousDrawSurface = EGL_NO_SURFACE;
    }
}

void QOpenVGOffscreenSurface::swapBuffers()
{
    eglSwapBuffers(m_display, m_renderTarget);
}

QImage QOpenVGOffscreenSurface::readbackQImage()
{
    QImage readbackImage(m_size, QImage::Format_ARGB32_Premultiplied);
    vgGetImageSubData(m_image, readbackImage.bits(), readbackImage.bytesPerLine(), VG_sARGB_8888_PRE, 0, 0, m_size.width(), m_size.height());
    return readbackImage;
}

QT_END_NAMESPACE
