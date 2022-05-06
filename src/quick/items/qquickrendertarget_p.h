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

#ifndef QQUICKRENDERTARGET_P_H
#define QQUICKRENDERTARGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/private/qtquickglobal_p.h>
#include "qquickrendertarget.h"
#include <QAtomicInt>

QT_BEGIN_NAMESPACE

class QRhi;
class QQuickWindowRenderTarget;

class Q_QUICK_PRIVATE_EXPORT QQuickRenderTargetPrivate
{
public:
    static QQuickRenderTargetPrivate *get(QQuickRenderTarget *rt) { return rt->d; }
    static const QQuickRenderTargetPrivate *get(const QQuickRenderTarget *rt) { return rt->d; }
    QQuickRenderTargetPrivate();
    QQuickRenderTargetPrivate(const QQuickRenderTargetPrivate *other);
    bool resolve(QRhi *rhi, QQuickWindowRenderTarget *dst);

    enum class Type {
        Null,
        NativeTexture,
        NativeRenderbuffer,
        RhiRenderTarget
    };

    QAtomicInt ref;
    Type type = Type::Null;
    QSize pixelSize;
    int sampleCount = 1;
    struct NativeTexture {
        quint64 object;
        int layout;
    };
    union {
        NativeTexture nativeTexture;
        quint64 nativeRenderbufferObject;
        QRhiRenderTarget *rhiRt;
    } u;
};

QT_END_NAMESPACE

#endif // QQUICKRENDERTARGET_P_H
