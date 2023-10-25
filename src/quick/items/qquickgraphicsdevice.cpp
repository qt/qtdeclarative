// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    \note It is up the caller to ensure that \a context is going to be
    compatible and usable with the QQuickWindow. Platform-specific mismatches in
    the associated QSurfaceFormat, or threading issues due to attempting to use
    \a context on multiple threads are up to the caller to avoid.
 */
#if QT_CONFIG(opengl) || defined(Q_QDOC)
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

    This factory function is suitable for Direct3D 11 and 12, particularly in
    combination with OpenXR. \a adapterLuidLow and \a adapterLuidHigh together
    specify a LUID, while a featureLevel specifies a \c{D3D_FEATURE_LEVEL_}
    value. \a featureLevel can be set to 0 if it is not intended to be
    specified, in which case the scene graph's defaults will be used.

    \note With Direct 3D 12 \a featureLevel specifies the \c minimum feature
    level passed on to D3D12CreateDevice().
 */
#if defined(Q_OS_WIN) || defined(Q_QDOC)
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

    It also supports Direct 3D 12, if that is the 3D API used at run time. With
    D3D12 \a context is unused and can be set to null. \a device is expected to
    be a \c{ID3D12Device*}.

    \note the resulting QQuickGraphicsDevice does not own any native resources,
    it merely contains references. It is the caller's responsibility to ensure
    that the native resource exists as long as necessary.
 */
#if defined(Q_OS_WIN) || defined(Q_QDOC)
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
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_QDOC)
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
#if QT_CONFIG(vulkan) || defined(Q_QDOC)
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
#if QT_CONFIG(vulkan) || defined(Q_QDOC)
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

/*!
    \return a new QQuickGraphicsDevice referencing an existing \a rhi object.

    \note Similarly to fromOpenGLContext(), the caller must be careful to only
    share a QRhi (and so the underlying graphics context or device) between
    QQuickWindows that are known to be compatible, not breaking the underlying
    graphics API's rules when it comes to threading, pixel formats, etc.

    \since 6.6
*/
QQuickGraphicsDevice QQuickGraphicsDevice::fromRhi(QRhi *rhi)
{
    QQuickGraphicsDevice dev;
    QQuickGraphicsDevicePrivate *d = QQuickGraphicsDevicePrivate::get(&dev);
    d->type = QQuickGraphicsDevicePrivate::Type::Rhi;
    d->u.rhi = rhi;
    return dev;
}

QQuickGraphicsDevicePrivate::QQuickGraphicsDevicePrivate()
    : ref(1)
{
}

QQuickGraphicsDevicePrivate::QQuickGraphicsDevicePrivate(const QQuickGraphicsDevicePrivate &other)
    : ref(1),
      type(other.type),
      u(other.u)
{
}

QT_END_NAMESPACE
