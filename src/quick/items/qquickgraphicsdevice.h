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

#ifndef QQUICKGRAPHICSDEVICE_H
#define QQUICKGRAPHICSDEVICE_H

#include <QtQuick/qtquickglobal.h>

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_CLANG_QDOC)
Q_FORWARD_DECLARE_OBJC_CLASS(MTLDevice);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLCommandQueue);
#endif

QT_BEGIN_NAMESPACE

class QQuickGraphicsDevicePrivate;
class QOpenGLContext;

class Q_QUICK_EXPORT QQuickGraphicsDevice
{
public:
    QQuickGraphicsDevice();
    ~QQuickGraphicsDevice();
    QQuickGraphicsDevice(const QQuickGraphicsDevice &other);
    QQuickGraphicsDevice &operator=(const QQuickGraphicsDevice &other);

    bool isNull() const;

#if QT_CONFIG(opengl) || defined(Q_CLANG_QDOC)
    static QQuickGraphicsDevice fromOpenGLContext(QOpenGLContext *context);
#endif

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
    static QQuickGraphicsDevice fromAdapter(quint32 adapterLuidLow, qint32 adapterLuidHigh, int featureLevel = 0);
    static QQuickGraphicsDevice fromDeviceAndContext(void *device, void *context);
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_CLANG_QDOC)
    static QQuickGraphicsDevice fromDeviceAndCommandQueue(MTLDevice *device, MTLCommandQueue *commandQueue);
#endif

#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)
    static QQuickGraphicsDevice fromPhysicalDevice(VkPhysicalDevice physicalDevice);
    static QQuickGraphicsDevice fromDeviceObjects(VkPhysicalDevice physicalDevice, VkDevice device, int queueFamilyIndex, int queueIndex = 0);
#endif

private:
    void detach();
    QQuickGraphicsDevicePrivate *d;
    friend class QQuickGraphicsDevicePrivate;
};

QT_END_NAMESPACE

#endif // QQUICKGRAPHICSDEVICE_H
