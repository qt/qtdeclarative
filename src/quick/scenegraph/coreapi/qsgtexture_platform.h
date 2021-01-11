/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGTEXTURE_PLATFORM_H
#define QSGTEXTURE_PLATFORM_H

#include <QtQuick/qquickwindow.h>

#if QT_CONFIG(opengl)
#include <QtGui/qopengl.h>
#endif

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

#if defined(__OBJC__) || defined(Q_CLANG_QDOC)
@protocol MTLTexture;
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface {

#if QT_CONFIG(opengl) || defined(Q_CLANG_QDOC)
struct Q_QUICK_EXPORT QSGOpenGLTexture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGOpenGLTexture)
    virtual GLuint nativeTexture() const = 0;
    static QSGTexture *fromNative(GLuint textureId,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
    static QSGTexture *fromNativeExternalOES(GLuint textureId,
                                             QQuickWindow *window,
                                             const QSize &size,
                                             QQuickWindow::CreateTextureOptions options = {});
};
#endif

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
struct Q_QUICK_EXPORT QSGD3D11Texture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGD3D11Texture)
    virtual void *nativeTexture() const = 0;
    static QSGTexture *fromNative(void *texture,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
};
#endif

#if defined(__OBJC__) || defined(Q_CLANG_QDOC)
struct Q_QUICK_EXPORT QSGMetalTexture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGMetalTexture)
    virtual id<MTLTexture> nativeTexture() const = 0;
    static QSGTexture *fromNative(id<MTLTexture> texture,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
};
#endif

#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)
struct Q_QUICK_EXPORT QSGVulkanTexture
{
    QT_DECLARE_NATIVE_INTERFACE(QSGVulkanTexture)
    virtual VkImage nativeImage() const = 0;
    virtual VkImageLayout nativeImageLayout() const = 0;
    static QSGTexture *fromNative(VkImage image,
                                  VkImageLayout layout,
                                  QQuickWindow *window,
                                  const QSize &size,
                                  QQuickWindow::CreateTextureOptions options = {});
};
#endif

} // QNativeInterface

QT_END_NAMESPACE

#endif // QSGTEXTURE_PLATFORM_H
