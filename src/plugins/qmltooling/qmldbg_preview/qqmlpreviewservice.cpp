// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlpreviewservice.h"
#include "qqmlclassicpreviewhandler.h"
#include "qqmlinplacepreviewhandler.h"

#include <QtCore/qpointer.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>
#include <QtGui/qguiapplication.h>

#include <private/qquickpixmap_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>

QT_BEGIN_NAMESPACE

const QString QQmlPreviewServiceImpl::s_key = QStringLiteral("QmlPreview");
using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

QQmlPreviewServiceImpl::QQmlPreviewServiceImpl(QObject *parent) :
    QQmlDebugService(s_key, 1.0f, parent),
    m_handler(std::make_unique<QQmlClassicPreviewHandler>())
{
    m_handler->connectToService(this);
}

QQmlPreviewServiceImpl::~QQmlPreviewServiceImpl()
{
}

void QQmlPreviewServiceImpl::switchToInPlaceHandler()
{
    // Transfer engines from the old handler to the new one
    const auto engineList = m_handler->engines();
    m_handler = std::make_unique<QQmlInPlacePreviewHandler>();
    m_handler->connectToService(this);
    for (QQmlEngine *engine : engineList)
        m_handler->addEngine(engine);

    // Send confirmation back to the client
    QQmlDebugPacket response;
    const bool enableInPlaceUpdates = true;
    response << static_cast<qint8>(Confirmation) << enableInPlaceUpdates;
    emit messageToClient(name(), response.data());
}

void QQmlPreviewServiceImpl::messageReceived(const QByteArray &data)
{
    QQmlDebugPacket packet(data);
    qint8 command;

    packet >> command;
    switch (command) {
    case File: {
        QString path;
        QByteArray contents;
        packet >> path >> contents;

        const QUrl url = path.startsWith(QLatin1Char(':'))
                ? QUrl(QLatin1String("qrc") + path)
                : QUrl::fromLocalFile(path);

        emit drop(url);
        emit file(path, contents);

        // Remember the first .qml file as the current URL, so that a Load command
        // without an explicit URL can fall back to it.
        if (m_currentUrl.isEmpty() && path.endsWith(".qml"))
            m_currentUrl = url;

        // Never auto-load here. The client must send an explicit Load command
        // to trigger loading. Auto-loading based on File responses is racy
        // (the Configuration message may not have arrived yet) and creates
        // duplicate objects when the host process has already loaded the scene.
        break;
    }
    case Directory: {
        QString path;
        QStringList entries;
        packet >> path >> entries;
        emit directory(path, entries);
        break;
    }
    case Load: {
        QUrl url;
        packet >> url;
        if (url.isEmpty())
            url = m_currentUrl;
        else
            m_currentUrl = url;
        emit load(url);
        break;
    }
    case Error: {
        QString file;
        packet >> file;
        emit error(file);
        break;
    }
    case Rerun:
        emit rerun();
        break;
    case ClearCache:
        emit clearCache();
        break;
    case Zoom: {
        float factor;
        packet >> factor;
        emit zoom(static_cast<qreal>(factor));
        break;
    }
    case AnimationSpeed: {
        float factor;
        packet >> factor;
        emit animationSpeed(qreal(factor));
        break;
    }
    case Configuration: {
        if (qEnvironmentVariableIsSet("QMLPREVIEW_HOTRELOAD")) {
            bool enableInPlaceUpdates;
            packet >> enableInPlaceUpdates;
            if (enableInPlaceUpdates
                    && !qobject_cast<QQmlInPlacePreviewHandler *>(m_handler.get())) {
                // Schedule this to the main thread where the handler lives
                // We're on the debug server thread here.
                QMetaObject::invokeMethod(this, &QQmlPreviewServiceImpl::switchToInPlaceHandler);
            } else if (!enableInPlaceUpdates
                       && qobject_cast<QQmlInPlacePreviewHandler *>(m_handler.get())) {
                forwardError(QLatin1String("Cannot disable in-place updates once enabled"));
            }
            break;
        } else {
            Q_FALLTHROUGH();
        }
    }
    default:
        forwardError(QString::fromLatin1("Invalid command: %1").arg(command));
        break;
    }
}

void QQmlPreviewServiceImpl::engineAboutToBeAdded(QJSEngine *engine)
{
    if (QQmlEngine *qmlEngine = qobject_cast<QQmlEngine *>(engine))
        m_handler->addEngine(qmlEngine);
    emit attachedToEngine(engine);
}

void QQmlPreviewServiceImpl::engineAboutToBeRemoved(QJSEngine *engine)
{
    if (QQmlEngine *qmlEngine = qobject_cast<QQmlEngine *>(engine))
        m_handler->removeEngine(qmlEngine);
    emit detachedFromEngine(engine);
}

void QQmlPreviewServiceImpl::stateChanged(QQmlDebugService::State state)
{
    if (state == Enabled) {
        m_loader.reset(new QQmlPreviewFileLoader(this));
        connect(this, &QQmlPreviewServiceImpl::load,
                m_loader.data(), &QQmlPreviewFileLoader::whitelist, Qt::DirectConnection);
        QV4::ExecutionEngine::setPreviewing(true);
        m_fileEngine.reset(new QQmlPreviewFileEngineHandler(m_loader.data()));
    } else {
        QV4::ExecutionEngine::setPreviewing(false);
        m_fileEngine.reset();
        m_loader.reset();
    }
}

void QQmlPreviewServiceImpl::forwardRequest(const QString &file)
{
    QQmlDebugPacket packet;
    packet << static_cast<qint8>(Request) << file;
    emit messageToClient(name(), packet.data());
}

void QQmlPreviewServiceImpl::forwardError(const QString &error)
{
    QQmlDebugPacket packet;
    packet << static_cast<qint8>(Error) << error;
    emit messageToClient(name(), packet.data());
}

void QQmlPreviewServiceImpl::forwardFps(const QQmlPreviewHandler::FpsInfo &frames)
{
    QQmlDebugPacket packet;
    packet << static_cast<qint8>(Fps)
           << frames.numSyncs << frames.minSync << frames.maxSync << frames.totalSync
           << frames.numRenders << frames.minRender << frames.maxRender << frames.totalRender;
    emit messageToClient(name(), packet.data());
}

void QQmlPreviewServiceImpl::forwardHotReloadFailure(const QString &reason)
{
    QQmlDebugPacket packet;
    packet << static_cast<qint8>(HotReloadFailure) << reason;
    emit messageToClient(name(), packet.data());
}

QQuickItem *QQmlPreviewServiceImpl::currentRootItem()
{
    return m_handler->currentRootItem();
}

QT_END_NAMESPACE

#include "moc_qqmlpreviewservice.cpp"
