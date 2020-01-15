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
    Constructs a default QQuickGraphicsDEvice that does not reference any native
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
    Constructs a default QQuickRenderTarget that does not reference any native
    objects.
 */
bool QQuickGraphicsDevice::isNull() const
{
    return d->type == QQuickGraphicsDevicePrivate::Type::Null;
}

/*!
    \return a new QQuickGraphicsDevice referencing an existing QOpenGLContext.

    This factory function is suitable for OpenGL.
 */
QQuickGraphicsDevice QQuickGraphicsDevice::fromOpenGLContext(QOpenGLContext *context)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::OpenGLContext;
    d->u.context = context;
    return dev;
}

/*!
    \return a new QQuickGraphicsDevice referencing a native device and context
    object.

    This factory function is suitable for:

    \list

    \li Direct3D11 - \a device is expected to be a \c{ID3D11Device*}, \a
    context is expected to be a \c{ID3D11DeviceContext*}.

    \endlist

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.

 */
QQuickGraphicsDevice QQuickGraphicsDevice::fromDeviceAndContext(void *device, void *context)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::DeviceAndContext;
    d->u.deviceAndContext = { device, context };
    return dev;
}

/*!
    \return a new QQuickGraphicsDevice referencing a native device and command
    queue object.

    This factory function is suitable for:

    \list

    \li Metal - \a device is expected to be a \c{MTLDevice*}, \a cmdQueue is
    expected to be a \c{MTLCommandQueue*}.

    \endlist

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.

 */
QQuickGraphicsDevice QQuickGraphicsDevice::fromDeviceAndCommandQueue(void *device, void *cmdQueue)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::DeviceAndCommandQueue;
    d->u.deviceAndCommandQueue = { device, cmdQueue };
    return dev;
}

/*!
    \return a new QQuickGraphicsDevice referencing a native device and related
    objects.

    This factory function is suitable for:

    \list

    \li Vulkan - \a physicalDevice is expected to be \c VkPhysicalDevice, \a
    device is expected to be a \a VkDevice, while \a queueFamilyIndex is the
    index of the graphics queue family on the device.

    \endlist

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.

 */
QQuickGraphicsDevice QQuickGraphicsDevice::fromDeviceObjects(void *physicalDevice, void *device, int queueFamilyIndex)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::DeviceObjects;
    d->u.deviceObjects = { physicalDevice, device, queueFamilyIndex };
    return dev;
}

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
