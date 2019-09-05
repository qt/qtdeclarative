/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qsgrhinativetextureimporter_p.h"
#include <private/qsgrhisupport_p.h> // to get all the relevant qrhi headers

QT_BEGIN_NAMESPACE

void QSGRhiNativeTextureImporter::buildWrapper(QRhi *rhi, QRhiTexture *t,
                                               const void *nativeObjectPtr, int nativeLayout)
{
#if !QT_CONFIG(vulkan)
    Q_UNUSED(nativeLayout);
#endif
#if !QT_CONFIG(opengl) && !QT_CONFIG(vulkan) && !defined(Q_OS_WIN) && !defined(Q_OS_DARWIN)
    Q_UNUSED(nativeObjectPtr);
#endif

    switch (rhi->backend()) {
    case QRhi::OpenGLES2:
    {
#if QT_CONFIG(opengl)
        QRhiGles2TextureNativeHandles h;
        h.texture = *reinterpret_cast<const uint *>(nativeObjectPtr);
        t->buildFrom(&h);
#endif
    }
        break;
    case QRhi::Vulkan:
    {
#if QT_CONFIG(vulkan)
        QRhiVulkanTextureNativeHandles h;
        h.image = *reinterpret_cast<const VkImage *>(nativeObjectPtr);
        h.layout = VkImageLayout(nativeLayout);
        t->buildFrom(&h);
#endif
    }
        break;
    case QRhi::D3D11:
    {
#ifdef Q_OS_WIN
        QRhiD3D11TextureNativeHandles h;
        h.texture = *reinterpret_cast<void * const *>(nativeObjectPtr);
        t->buildFrom(&h);
#endif
    }
        break;
    case QRhi::Metal:
    {
#ifdef Q_OS_DARWIN
        QRhiMetalTextureNativeHandles h;
        h.texture = *reinterpret_cast<void * const *>(nativeObjectPtr);
        t->buildFrom(&h);
#endif
    }
        break;
    case QRhi::Null:
        t->build();
        break;
    default:
        qWarning("QSGRhiNativeTextureImporter: encountered an unsupported QRhi backend (%d)",
                 int(rhi->backend()));
        t->build();
        break;
    }
}

QT_END_NAMESPACE
