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

#include "qquickdesignerwindowmanager_p.h"
#include "private/qquickwindow_p.h"
#include <QtQuick/QQuickWindow>
#if QT_CONFIG(opengl)
#include <private/qsgdefaultrendercontext_p.h>
#endif

QT_BEGIN_NAMESPACE

QQuickDesignerWindowManager::QQuickDesignerWindowManager()
    : m_sgContext(QSGContext::createDefaultContext())
{
    m_renderContext.reset(m_sgContext.data()->createRenderContext());
}

void QQuickDesignerWindowManager::show(QQuickWindow *window)
{
    makeOpenGLContext(window);
}

void QQuickDesignerWindowManager::hide(QQuickWindow *)
{
}

void QQuickDesignerWindowManager::windowDestroyed(QQuickWindow *)
{
}

void QQuickDesignerWindowManager::makeOpenGLContext(QQuickWindow *window)
{
#if QT_CONFIG(opengl)
    if (!m_openGlContext) {
        m_openGlContext.reset(new QOpenGLContext());
        m_openGlContext->setFormat(window->requestedFormat());
        m_openGlContext->create();
        if (!m_openGlContext->makeCurrent(window))
            qWarning("QQuickWindow: makeCurrent() failed...");
        QSGDefaultRenderContext::InitParams params;
        params.sampleCount = qMax(1, m_openGlContext->format().samples());
        params.openGLContext = m_openGlContext.data();
        params.initialSurfacePixelSize = window->size() * window->effectiveDevicePixelRatio();
        params.maybeSurface = window;
        m_renderContext->initialize(&params);
    } else {
        m_openGlContext->makeCurrent(window);
    }
#else
    Q_UNUSED(window)
#endif
}

void QQuickDesignerWindowManager::exposureChanged(QQuickWindow *)
{
}

QImage QQuickDesignerWindowManager::grab(QQuickWindow *)
{
    return QImage();
}

void QQuickDesignerWindowManager::maybeUpdate(QQuickWindow *)
{
}

QSGContext *QQuickDesignerWindowManager::sceneGraphContext() const
{
    return m_sgContext.data();
}

void QQuickDesignerWindowManager::createOpenGLContext(QQuickWindow *window)
{
    window->create();
    window->update();
}

void QQuickDesignerWindowManager::update(QQuickWindow *window)
{
    makeOpenGLContext(window);
}

QT_END_NAMESPACE


#include "moc_qquickdesignerwindowmanager_p.cpp"
