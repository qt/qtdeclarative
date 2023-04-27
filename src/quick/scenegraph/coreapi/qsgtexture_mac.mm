// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgtexture_p.h"
#include "qsgtexture_platform.h"
#include <private/qquickitem_p.h>
#include <private/qquickwindow_p.h>
#include <QtCore/private/qnativeinterface_p.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

id<MTLTexture> QSGTexturePlatformMetal::nativeTexture() const
{
    if (auto *tex = m_texture->rhiTexture())
        return (id<MTLTexture>) quintptr(tex->nativeTexture().object);
    return 0;
}

namespace QNativeInterface {

QT_DEFINE_NATIVE_INTERFACE(QSGMetalTexture);

QSGTexture *QSGMetalTexture::fromNative(id<MTLTexture> texture,
                                        QQuickWindow *window,
                                        const QSize &size,
                                        QQuickWindow::CreateTextureOptions options)
{
    return QQuickWindowPrivate::get(window)->createTextureFromNativeTexture(quint64(texture), 0, size, options);
}

} // QNativeInterface

QT_END_NAMESPACE
