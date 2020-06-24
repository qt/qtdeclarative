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

#ifndef QQUICKGRAPHICSDEVICE_P_H
#define QQUICKGRAPHICSDEVICE_P_H

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
#include <QAtomicInt>
#include "qquickgraphicsdevice.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickGraphicsDevicePrivate
{
public:
    static QQuickGraphicsDevicePrivate *get(QQuickGraphicsDevice *p) { return p->d; }
    static const QQuickGraphicsDevicePrivate *get(const QQuickGraphicsDevice *p) { return p->d; }
    QQuickGraphicsDevicePrivate();
    QQuickGraphicsDevicePrivate(const QQuickGraphicsDevicePrivate *other);

    enum class Type {
        Null,
        OpenGLContext,
        Adapter,
        DeviceAndContext,
        DeviceAndCommandQueue,
        PhysicalDevice,
        DeviceObjects
    };

    QAtomicInt ref;
    Type type = Type::Null;

    struct Adapter {
        quint32 luidLow;
        qint32 luidHigh;
        int featureLevel;
    };

    struct DeviceAndContext {
        void *device;
        void *context;
    };

    struct DeviceAndCommandQueue {
        void *device;
        void *cmdQueue;
    };

    struct PhysicalDevice {
        void *physicalDevice;
    };

    struct DeviceObjects {
        void *physicalDevice;
        void *device;
        int queueFamilyIndex;
        int queueIndex;
    };

    union {
        QOpenGLContext *context;
        Adapter adapter;
        DeviceAndContext deviceAndContext;
        DeviceAndCommandQueue deviceAndCommandQueue;
        PhysicalDevice physicalDevice;
        DeviceObjects deviceObjects;
    } u;
};

QT_END_NAMESPACE

#endif // QQUICKGRAPHICSDEVICE_P_H
