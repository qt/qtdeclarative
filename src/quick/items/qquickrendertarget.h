// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRENDERTARGET_H
#define QQUICKRENDERTARGET_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/qsize.h>

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_QDOC)
Q_FORWARD_DECLARE_OBJC_CLASS(MTLTexture);
#endif

QT_BEGIN_NAMESPACE

class QQuickRenderTargetPrivate;
class QRhiRenderTarget;
class QPaintDevice;

class Q_QUICK_EXPORT QQuickRenderTarget
{
public:
    QQuickRenderTarget();
    ~QQuickRenderTarget();
    QQuickRenderTarget(const QQuickRenderTarget &other);
    QQuickRenderTarget &operator=(const QQuickRenderTarget &other);

    bool isNull() const;

    qreal devicePixelRatio() const;
    void setDevicePixelRatio(qreal ratio);

    bool mirrorVertically() const;
    void setMirrorVertically(bool enable);

#if QT_CONFIG(opengl) || defined(Q_QDOC)
    static QQuickRenderTarget fromOpenGLTexture(uint textureId, uint format, const QSize &pixelSize, int sampleCount = 1);
    static QQuickRenderTarget fromOpenGLTexture(uint textureId, const QSize &pixelSize, int sampleCount = 1);
    static QQuickRenderTarget fromOpenGLRenderBuffer(uint renderbufferId, const QSize &pixelSize, int sampleCount = 1);
#endif

#if defined(Q_OS_WIN) || defined(Q_QDOC)
    static QQuickRenderTarget fromD3D11Texture(void *texture, uint format, const QSize &pixelSize, int sampleCount = 1);
    static QQuickRenderTarget fromD3D11Texture(void *texture, const QSize &pixelSize, int sampleCount = 1);
    static QQuickRenderTarget fromD3D12Texture(void *texture, int resourceState, uint format, const QSize &pixelSize, int sampleCount = 1);
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_QDOC)
    static QQuickRenderTarget fromMetalTexture(MTLTexture *texture, uint format, const QSize &pixelSize, int sampleCount = 1);
    static QQuickRenderTarget fromMetalTexture(MTLTexture *texture, const QSize &pixelSize, int sampleCount = 1);
#endif

#if QT_CONFIG(vulkan) || defined(Q_QDOC)
    static QQuickRenderTarget fromVulkanImage(VkImage image, VkImageLayout layout, VkFormat format, const QSize &pixelSize, int sampleCount = 1);
    static QQuickRenderTarget fromVulkanImage(VkImage image, VkImageLayout layout, const QSize &pixelSize, int sampleCount = 1);
#endif

    static QQuickRenderTarget fromRhiRenderTarget(QRhiRenderTarget *renderTarget);

    static QQuickRenderTarget fromPaintDevice(QPaintDevice *device);

private:
    void detach();
    bool isEqual(const QQuickRenderTarget &other) const noexcept;
    QQuickRenderTargetPrivate *d;
    friend class QQuickRenderTargetPrivate;

    friend bool operator==(const QQuickRenderTarget &lhs, const QQuickRenderTarget &rhs) noexcept
    { return lhs.isEqual(rhs); }
    friend bool operator!=(const QQuickRenderTarget &lhs, const QQuickRenderTarget &rhs) noexcept
    { return !lhs.isEqual(rhs); }
};

QT_END_NAMESPACE

#endif // QQUICKRENDERTARGET_H
