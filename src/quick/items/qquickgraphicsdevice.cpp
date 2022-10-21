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

#include "qquickgraphicsdevice_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QQuickGraphicsDevice
    \since 6.0
    \inmodule QtQuick

    \brief The QQuickGraphicsDevice class provides an opaque container for
    native graphics objects representing graphics devices or contexts.

    \sa QQuickWindow::setGraphicsDevice(), QQuickRenderTarget
*/

/*!
    Constructs a default QQuickGraphicsDevice that does not reference any native
    objects.
 */
QQuickGraphicsDevice::QQuickGraphicsDevice()
    : d(new QQuickGraphicsDevicePrivate)
{
}

/*!
    \internal
 */
void QQuickGraphicsDevice::detach()
{
    qAtomicDetach(d);
}

/*!
    \internal
 */
QQuickGraphicsDevice::QQuickGraphicsDevice(const QQuickGraphicsDevice &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    \internal
 */
QQuickGraphicsDevice &QQuickGraphicsDevice::operator=(const QQuickGraphicsDevice &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    Destructor.
 */
QQuickGraphicsDevice::~QQuickGraphicsDevice()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \return true if this is a default constructed graphics device that
    does not reference any native objects.
 */
bool QQuickGraphicsDevice::isNull() const
{
    return d->type == QQuickGraphicsDevicePrivate::Type::Null;
}

/*!
    \return a new QQuickGraphicsDevice referencing an existing OpenGL \a context.

    This factory function is suitable for OpenGL.
 */
#if QT_CONFIG(opengl) || defined(Q_CLANG_QDOC)
QQuickGraphicsDevice QQuickGraphicsDevice::fromOpenGLContext(QOpenGLContext *context)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::OpenGLContext;
    d->u.context = context;
    return dev;
}
#endif

/*!
    \return a new QQuickGraphicsDevice describing a DXGI adapter and D3D feature level.

    This factory function is suitable for Direct3D 11, particularly in
    combination with OpenXR. \a adapterLuidLow and \a adapterLuidHigh together
    specify a LUID, while a featureLevel specifies a \c{D3D_FEATURE_LEVEL_}
    value. \a featureLevel can be set to 0 if it is not intended to be
    specified, in which case the scene graph's defaults will be used.
 */
#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
QQuickGraphicsDevice QQuickGraphicsDevice::fromAdapter(quint32 adapterLuidLow,
                                                       qint32 adapterLuidHigh,
                                                       int featureLevel)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::Adapter;
    d->u.adapter = { adapterLuidLow, adapterLuidHigh, featureLevel };
    return dev;
}
#endif

/*!
    \return a new QQuickGraphicsDevice referencing a native device and context
    object.

    This factory function is suitable for Direct3D 11. \a device is expected to
    be a \c{ID3D11Device*}, \a context is expected to be a
    \c{ID3D11DeviceContext*}.

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.
 */
#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
QQuickGraphicsDevice QQuickGraphicsDevice::fromDeviceAndContext(void *device, void *context)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::DeviceAndContext;
    d->u.deviceAndContext = { device, context };
    return dev;
}
#endif

/*!
    \return a new QQuickGraphicsDevice referencing an existing \a device and
    \a commandQueue object.

    This factory function is suitable for Metal.

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.

 */
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_CLANG_QDOC)
QQuickGraphicsDevice QQuickGraphicsDevice::fromDeviceAndCommandQueue(MTLDevice *device,
                                                                     MTLCommandQueue *commandQueue)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::DeviceAndCommandQueue;
    d->u.deviceAndCommandQueue = { device, commandQueue };
    return dev;
}
#endif

/*!
    \return a new QQuickGraphicsDevice referencing an existing \a physicalDevice.

    This factory function is suitable for Vulkan, particularly in combination
    with OpenXR.

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.
 */
#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)
QQuickGraphicsDevice QQuickGraphicsDevice::fromPhysicalDevice(VkPhysicalDevice physicalDevice)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::PhysicalDevice;
    d->u.physicalDevice = { physicalDevice };
    return dev;
}
#endif

/*!
    \return a new QQuickGraphicsDevice referencing an existing \a device object.

    This factory function is suitable for Vulkan. \a physicalDevice, \a device
    and \a queueFamilyIndex must always be provided. \a queueIndex is optional
    since the default value of 0 is often suitable.

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.
 */
#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)
QQuickGraphicsDevice QQuickGraphicsDevice::fromDeviceObjects(VkPhysicalDevice physicalDevice,
                                                             VkDevice device,
                                                             int queueFamilyIndex,
                                                             int queueIndex)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::DeviceObjects;
    d->u.deviceObjects = { physicalDevice, device, queueFamilyIndex, queueIndex };
    return dev;
}
#endif

QQuickGraphicsDevicePrivate::QQuickGraphicsDevicePrivate()
    : ref(1)
{
}

QQuickGraphicsDevicePrivate::QQuickGraphicsDevicePrivate(const QQuickGraphicsDevicePrivate *other)
    : ref(1),
      type(other->type),
      u(other->u)
{
}

QT_END_NAMESPACE
