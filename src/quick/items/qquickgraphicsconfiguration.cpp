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

#include "qquickgraphicsconfiguration_p.h"

#if QT_CONFIG(vulkan)
#include <QtGui/private/qrhivulkan_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QQuickGraphicsConfiguration
    \since 6.0
    \inmodule QtQuick

    \brief The QQuickGraphicsConfiguration class is a container for low-level
    graphics settings that can affect how the underlying graphics API, such as
    Vulkan, is initialized by the Qt Quick scene graph. It can also control
    certain aspects of the scene graph renderer.

    When constructing and showing a QQuickWindow that uses Vulkan to render, a
    Vulkan instance (\c VkInstance), a physical device (\c VkPhysicalDevice), a
    device (\c VkDevice) and associated objects (queues, pools) are initialized
    through the Vulkan API. The same is mostly true when using
    QQuickRenderControl to redirect the rendering into a custom render target,
    such as a texture. While QVulkanInstance construction is under the
    application's control then, the initialization of other graphics objects
    happen the same way in QQuickRenderControl::initialize() as with an
    on-screen QQuickWindow.

    For the majority of applications no additional configuration is needed
    because Qt Quick provides reasonable defaults for many low-level graphics
    settings, for example which device extensions to enable.

    This will not alway be sufficient, however. In advanced use cases, when
    integrating direct Vulkan or other graphics API content, or when
    integrating with an external 3D or VR engine, such as, OpenXR, the
    application will want to specify its own set of settings when it comes to
    details, such as which device extensions to enable.

    That is what this class enables. It allows specifying, for example, a list
    of device extensions that is then picked up by the scene graph when using
    Vulkan, or graphics APIs where the concept is applicable. Where some
    concepts are not applicable, the related settings are simply ignored.

    Another class of settings are related to the scene graph's renderer. In
    some cases applications may want to control certain behavior,such as using
    the depth buffer when rendering 2D content. In Qt 5 such settings were
    either not controllable at all, or were managed through environment
    variables. In Qt 6, QQuickGraphicsConfiguration provides a new home for
    these settings, while keeping support for the legacy environment variables,
    where applicable.

    \note Setting a QQuickGraphicsConfiguration on a QQuickWindow must happen
    early enough, before the scene graph is initialized for the first time for
    that window. With on-screen windows this means the call must be done before
    invoking show() on the QQuickWindow or QQuickView. With QQuickRenderControl
    the configuration must be finalized before calling
    \l{QQuickRenderControl::initialize()}{initialize()}.

    \sa QQuickWindow::setGraphicsConfiguration(), QQuickWindow, QQuickRenderControl
*/

/*!
    Constructs a default QQuickGraphicsConfiguration that does not specify any
    additional settings for the scene graph to take into account.
 */
QQuickGraphicsConfiguration::QQuickGraphicsConfiguration()
    : d(new QQuickGraphicsConfigurationPrivate)
{
}

/*!
    \internal
 */
void QQuickGraphicsConfiguration::detach()
{
    qAtomicDetach(d);
}

/*!
    \internal
 */
QQuickGraphicsConfiguration::QQuickGraphicsConfiguration(const QQuickGraphicsConfiguration &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    \internal
 */
QQuickGraphicsConfiguration &QQuickGraphicsConfiguration::operator=(const QQuickGraphicsConfiguration &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    Destructor.
 */
QQuickGraphicsConfiguration::~QQuickGraphicsConfiguration()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \return the list of Vulkan instance extensions Qt Quick prefers to
    have enabled on the VkInstance.

    In most cases Qt Quick is responsible for creating a QVulkanInstance. This
    function is not relevant then. On the other hand, when using
    QQuickRenderControl in combination with Vulkan-based rendering, it is the
    application's responsibility to create a QVulkanInstance and associate it
    with the (offscreen) QQuickWindow. In this case, it is expected that the
    application queries the list of instance extensions to enable, and passes
    them to QVulkanInstance::setExtensions() before calling
    QVulkanInstance::create().

    \since 6.1
 */
QByteArrayList QQuickGraphicsConfiguration::preferredInstanceExtensions()
{
#if QT_CONFIG(vulkan)
    return QRhiVulkanInitParams::preferredInstanceExtensions();
#else
    return {};
#endif
}

/*!
    Sets the list of additional \a extensions to enable on the graphics device
    (such as, the \c VkDevice).

    When rendering with a graphics API where the concept is not applicable, \a
    extensions will be ignored.

    \note The list specifies additional, extra extensions. Qt Quick always
    enables extensions that are required by the scene graph.
 */
void QQuickGraphicsConfiguration::setDeviceExtensions(const QByteArrayList &extensions)
{
    if (d->deviceExtensions != extensions) {
        detach();
        d->deviceExtensions = extensions;
    }
}

/*!
    Sets the usage of depth buffer for 2D content to \a enable. When disabled,
    the Qt Quick scene graph never writes into the depth buffer.

    By default the value is true, unless the \c{QSG_NO_DEPTH_BUFFER}
    environment variable is set.

    The default value of true is the most optimal setting for the vast majority
    of scenes. Disabling depth buffer usage reduces the efficiency of the scene
    graph's batching.

    There are cases however, when allowing the 2D content write to the depth
    buffer is not ideal. Consider a 3D scene as an "overlay" on top the 2D
    scene, rendered via Qt Quick 3D using a \l View3D with
    \l{View3D::renderMode}{renderMode} set to \c Overlay. In this case, having
    the depth buffer filled by 2D content can cause unexpected results. This is
    because the way the 2D scene graph renderer generates and handles depth
    values is not necessarily compatible with how a 3D scene works. This may end
    up in depth value clashes, collisions, and unexpected depth test
    failures. Therefore, the robust approach here is to call this function with
    \a enable set to false, and disable depth buffer writes for the 2D content
    in the QQuickWindow.

    \note This flag is not fully identical to setting the
    \c{QSG_NO_DEPTH_BUFFER} environment variable. This flag does not control the
    depth-stencil buffers' presence. It is rather relevant for the rendering
    pipeline. To force not having depth/stencil attachments at all, set
    \c{QSG_NO_DEPTH_BUFFER} and \c{QSG_NO_STENCIL_BUFFER}. Be aware however
    that such a QQuickWindow, and any Item layers in it, may then become
    incompatible with items, such as View3D with certain operating modes,
    because 3D content requires a depth buffer. Calling this function is always
    safe, but can mean that resources, such as depth buffers, are created even
    though they are not actively used.
 */
void QQuickGraphicsConfiguration::setDepthBufferFor2D(bool enable)
{
    if (d->useDepthBufferFor2D != enable) {
        detach();
        d->useDepthBufferFor2D = enable;
    }
}

/*!
    \return true if depth buffer usage is enabled for 2D content.

    By default the value is true, unless the \c{QSG_NO_DEPTH_BUFFER}
    environment variable is set.
 */
bool QQuickGraphicsConfiguration::isDepthBufferEnabledFor2D() const
{
    return d->useDepthBufferFor2D;
}

/*!
    \return the list of the requested additional device extensions.
 */
QByteArrayList QQuickGraphicsConfiguration::deviceExtensions() const
{
    return d->deviceExtensions;
}

QQuickGraphicsConfigurationPrivate::QQuickGraphicsConfigurationPrivate()
    : ref(1)
{
    static const bool useDepth = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
    useDepthBufferFor2D = useDepth;
}

QQuickGraphicsConfigurationPrivate::QQuickGraphicsConfigurationPrivate(const QQuickGraphicsConfigurationPrivate *other)
    : ref(1),
      deviceExtensions(other->deviceExtensions),
      useDepthBufferFor2D(other->useDepthBufferFor2D)
{
}

QT_END_NAMESPACE
