/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QSGRHISUPPORT_P_H
#define QSGRHISUPPORT_P_H

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

#include "qsgrenderloop_p.h"
#include "qsgrendererinterface.h"

#include <QtGui/private/qrhi_p.h>

#include <QtGui/private/qrhinull_p.h>

#if QT_CONFIG(opengl)
#include <QtGui/private/qrhigles2_p.h>
#endif

#if QT_CONFIG(vulkan)
#include <QtGui/private/qrhivulkan_p.h>
#endif

#ifdef Q_OS_WIN
#include <QtGui/private/qrhid3d11_p.h>
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <QtGui/private/qrhimetal_p.h>
#endif

#if QT_CONFIG(qml_network)
#define RHI_REMOTE_PROFILER
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtGui/private/qrhiprofiler_p.h>
#endif

QT_BEGIN_NAMESPACE

class QSGDefaultRenderContext;
class QVulkanInstance;
class QOffscreenSurface;

// Opting in/out of QRhi and choosing the default/requested backend is managed
// by this singleton. This is because this information may be needed before
// creating a render loop. A well-written render loop sets up its QRhi and
// related machinery using the helper functions in here.
//
// cleanup() must be called to perform global (not per thread) cleanup, such
// as, destroying the QVulkanInstance (if one was created in vulkanInstance()).
//
// In addition, the class provides handy conversion and query stuff for the
// renderloop and the QSGRendererInterface implementations.
//
class Q_QUICK_PRIVATE_EXPORT QSGRhiSupport
{
public:
    static QSGRhiSupport *instance_internal();
    static QSGRhiSupport *instance();
    static QVulkanInstance *defaultVulkanInstance();
    static void cleanupDefaultVulkanInstance();
    static int chooseSampleCountForWindowWithRhi(QWindow *window, QRhi *rhi);
    static QImage grabAndBlockInCurrentFrame(QRhi *rhi, QRhiCommandBuffer *cb, QRhiTexture *src = nullptr);
    static void checkEnvQSgInfo();

    void configure(QSGRendererInterface::GraphicsApi api);

    bool isRhiEnabled() const { return m_enableRhi; }
    QRhi::Implementation rhiBackend() const { return m_rhiBackend; }
    QString rhiBackendName() const;
    QSGRendererInterface::GraphicsApi graphicsApi() const;

    bool isDebugLayerRequested() const { return m_debugLayer; }
    bool isProfilingRequested() const { return m_profile; }
    bool isShaderEffectDebuggingRequested() const { return m_shaderEffectDebug; }
    bool isSoftwareRendererRequested() const { return m_preferSoftwareRenderer; }

    QSurface::SurfaceType windowSurfaceType() const;

    const void *rifResource(QSGRendererInterface::Resource res,
                            const QSGDefaultRenderContext *rc,
                            const QQuickWindow *w);

    QOffscreenSurface *maybeCreateOffscreenSurface(QWindow *window);
    QRhi *createRhi(QQuickWindow *window, QOffscreenSurface *offscreenSurface);
    void destroyRhi(QRhi *rhi);
    void prepareWindowForRhi(QQuickWindow *window);

    QImage grabOffscreen(QQuickWindow *window);

private:
    QSGRhiSupport();
    void applySettings();
    void adjustToPlatformQuirks();
    struct {
        bool valid = false;
        QSGRendererInterface::GraphicsApi api;
    } m_requested;
    QRhi::Implementation m_rhiBackend = QRhi::Null;
    int m_killDeviceFrameCount;
    QString m_pipelineCacheSave;
    QString m_pipelineCacheLoad;
    uint m_settingsApplied : 1;
    uint m_enableRhi : 1;
    uint m_debugLayer : 1;
    uint m_profile : 1;
    uint m_shaderEffectDebug : 1;
    uint m_preferSoftwareRenderer : 1;
};

// Sends QRhi resource statistics over a QTcpSocket. To be initialized by the
// renderloop when QSGRhiSupport::isProfilingRequested() is true. From the
// applications' side this is enabled by setting the env.vars. QSG_RHI_PROFILE=1
// and QSG_RHI_PROFILE_HOST=<address>. For security, this is also tied to
// CONFIG+=qml_debug in the application (just like QML debugging), so it won't
// be doing anything otherwise, even if the env vars are set.
class QSGRhiProfileConnection
{
public:
    static QSGRhiProfileConnection *instance();
    void initialize(QRhi *rhi);
    void cleanup();
    void send(QRhi *rhi);

private:
#ifdef RHI_REMOTE_PROFILER
    QScopedPointer<QTcpSocket> m_profConn;
    QElapsedTimer m_lastMemStatWrite;
#endif
};

QT_END_NAMESPACE

#endif // QSGRHISUPPORT_P_H
