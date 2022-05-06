/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "qsgtexture_p.h"
#include "qsgtexture_platform.h"
#include <private/qquickitem_p.h>
#include <private/qquickwindow_p.h>
#include <QtCore/private/qnativeinterface_p.h>
#include <QtGui/private/qrhi_p.h>

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
