// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgcontextplugin_p.h"
#include <QtQuick/private/qsgcontext_p.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/qlibraryinfo.h>

// Built-in adaptations
#include <QtQuick/private/qsgsoftwareadaptation_p.h>
#include <QtQuick/private/qsgdefaultcontext_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_INFO)

QSGContextPlugin::QSGContextPlugin(QObject *parent)
    : QObject(parent)
{
}

QSGContextPlugin::~QSGContextPlugin()
{
}

#if QT_CONFIG(library)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QSGContextFactoryInterface_iid, QLatin1String("/scenegraph")))
#endif

struct QSGAdaptationBackendData
{
    QSGAdaptationBackendData();
    ~QSGAdaptationBackendData();
    Q_DISABLE_COPY(QSGAdaptationBackendData)

    bool tried = false;
    QSGContextFactoryInterface *factory = nullptr;
    QString name;
    QSGContextFactoryInterface::Flags flags;

    QVector<QSGContextFactoryInterface *> builtIns;

    QString quickWindowBackendRequest;
};

QSGAdaptationBackendData::QSGAdaptationBackendData()
{
    // Fill in the table with the built-in adaptations.
    builtIns.append(new QSGSoftwareAdaptation);
}

QSGAdaptationBackendData::~QSGAdaptationBackendData()
{
    qDeleteAll(builtIns);
}

Q_GLOBAL_STATIC(QSGAdaptationBackendData, qsg_adaptation_data)

// This only works when the backend is loaded (contextFactory() was called),
// otherwise the return value is 0.
//
// Note that the default (OpenGL) implementation always results in 0, custom flags
// can only be returned from the other (either compiled-in or plugin-based) backends.
QSGContextFactoryInterface::Flags qsg_backend_flags()
{
    return qsg_adaptation_data()->flags;
}

QSGAdaptationBackendData *contextFactory()
{
    QSGAdaptationBackendData *backendData = qsg_adaptation_data();

    if (!backendData->tried) {
        backendData->tried = true;

        const QStringList args = QGuiApplication::arguments();
        QString requestedBackend = backendData->quickWindowBackendRequest; // empty or set via QQuickWindow::setSceneGraphBackend()

        for (int index = 0; index < args.size(); ++index) {
            if (args.at(index).startsWith(QLatin1String("--device="))) {
                requestedBackend = args.at(index).mid(9);
                break;
            }
        }

        if (requestedBackend.isEmpty())
            requestedBackend = qEnvironmentVariable("QMLSCENE_DEVICE");

        // A modern alternative. Scenegraph adaptations can represent backends
        // for different graphics APIs as well, instead of being specific to
        // some device or platform.
        if (requestedBackend.isEmpty())
            requestedBackend = qEnvironmentVariable("QT_QUICK_BACKEND");

        // If this platform does not support OpenGL, Vulkan, D3D11, or Metal, and no
        // backend has been set, default to the software renderer. We rely on the
        // static, build time flags only. This is to prevent the inevitable confusion
        // caused by run time hocus pocus. If one wants to use the software backend
        // in a GL or Vulkan capable Qt build (or on Windows or Apple platforms), it
        // has to be requested explicitly.
#if !QT_CONFIG(opengl) && !QT_CONFIG(vulkan) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_IOS)
        if (requestedBackend.isEmpty())
            requestedBackend = QLatin1String("software");
#endif

        // As an exception to the above, play nice with platform plugins like
        // vnc or linuxfb: Trying to initialize a QRhi is futile on these, and
        // Qt 5 had an explicit fallback to the software backend, based on the
        // OpenGL capability. Replicate that behavior using the new
        // RhiBasedRendering capability flag, which, on certain platforms,
        // indicates that we should not even bother trying to initialize a QRhi
        // as no 3D API can be expected work.
        if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering)) {
            if (requestedBackend.isEmpty())
                requestedBackend = QLatin1String("software");
        }

        // This is handy if some of the logic above goes wrong and we select
        // e.g. the software backend when it is not desired.
        if (requestedBackend == QLatin1String("rhi"))
            requestedBackend.clear(); // empty = no custom backend to load

        if (!requestedBackend.isEmpty()) {
            qCDebug(QSG_LOG_INFO, "Loading backend %s", qUtf8Printable(requestedBackend));

            // First look for a built-in adaptation.
            for (QSGContextFactoryInterface *builtInBackend : std::as_const(backendData->builtIns)) {
                if (builtInBackend->keys().contains(requestedBackend)) {
                    backendData->factory = builtInBackend;
                    backendData->name = requestedBackend;
                    backendData->flags = backendData->factory->flags(requestedBackend);
                    break;
                }
            }

#if QT_CONFIG(library)
            // Then try the plugins.
            if (!backendData->factory) {
                const int index = loader()->indexOf(requestedBackend);
                if (index != -1)
                    backendData->factory = qobject_cast<QSGContextFactoryInterface*>(loader()->instance(index));
                if (backendData->factory) {
                    backendData->name = requestedBackend;
                    backendData->flags = backendData->factory->flags(requestedBackend);
                }
                if (!backendData->factory) {
                    qWarning("Could not create scene graph context for backend '%s'"
                             " - check that plugins are installed correctly in %s",
                             qPrintable(requestedBackend),
                             qPrintable(QLibraryInfo::path(QLibraryInfo::PluginsPath)));
                }
            }
#endif // library
        }
    }

    return backendData;
}



/*!
    \fn QSGContext *QSGContext::createDefaultContext()

    Creates a default scene graph context for the current hardware.
    This may load a device-specific plugin.
*/
QSGContext *QSGContext::createDefaultContext()
{
    QSGAdaptationBackendData *backendData = contextFactory();
    if (backendData->factory)
        return backendData->factory->create(backendData->name);
    return new QSGDefaultContext();
}



/*!
    Calls into the scene graph adaptation if available and creates a texture
    factory. The primary purpose of this function is to reimplement hardware
    specific asynchronous texture frameskip-less uploads that can happen on
    the image providers thread.
 */

QQuickTextureFactory *QSGContext::createTextureFactoryFromImage(const QImage &image)
{
    QSGAdaptationBackendData *backendData = contextFactory();
    if (backendData->factory)
        return backendData->factory->createTextureFactoryFromImage(image);
    return nullptr;
}


/*!
    Calls into the scene graph adaptation if available and creates a hardware
    specific window manager.
 */

QSGRenderLoop *QSGContext::createWindowManager()
{
    QSGAdaptationBackendData *backendData = contextFactory();
    if (backendData->factory)
        return backendData->factory->createWindowManager();
    return nullptr;
}

void QSGContext::setBackend(const QString &backend)
{
    QSGAdaptationBackendData *backendData = qsg_adaptation_data();
    if (backendData->tried)
        qWarning("Scenegraph already initialized, setBackend() request ignored");

    backendData->quickWindowBackendRequest = backend;
}

QString QSGContext::backend()
{
    QSGAdaptationBackendData *backendData = qsg_adaptation_data();
    if (backendData->tried)
        return backendData->name;

    return backendData->quickWindowBackendRequest;
}

QT_END_NAMESPACE

#include "moc_qsgcontextplugin_p.cpp"
