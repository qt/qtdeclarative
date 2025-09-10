// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpreviewservice.h"

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
    QQmlDebugService(s_key, 1.0f, parent)
{
    connect(this, &QQmlPreviewServiceImpl::load, &m_handler, &QQmlPreviewHandler::loadUrl);
    connect(this, &QQmlPreviewServiceImpl::rerun, &m_handler, &QQmlPreviewHandler::rerun);
    connect(this, &QQmlPreviewServiceImpl::zoom, &m_handler, &QQmlPreviewHandler::zoom);
    connect(&m_handler, &QQmlPreviewHandler::error, this, &QQmlPreviewServiceImpl::forwardError,
            Qt::DirectConnection);
    connect(&m_handler, &QQmlPreviewHandler::fps, this, &QQmlPreviewServiceImpl::forwardFps,
            Qt::DirectConnection);
}

QQmlPreviewServiceImpl::~QQmlPreviewServiceImpl()
{
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

        // Drop any existing compilation units for this URL from the type registry.
        if (const auto cu = QQmlMetaType::obtainCompilationUnit(url))
            QQmlMetaType::unregisterInternalCompositeType(cu);

        emit file(path, contents);

        // Replace the whole scene with the first file successfully loaded over the debug
        // connection. This is an OK approximation of the root component, and if the client wants
        // something specific, it will send an explicit Load anyway.
        if (m_currentUrl.isEmpty() && path.endsWith(".qml")) {
            m_currentUrl = url;
            emit load(m_currentUrl);
        }
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
    default:
        forwardError(QString::fromLatin1("Invalid command: %1").arg(command));
        break;
    }
}

void QQmlPreviewServiceImpl::engineAboutToBeAdded(QJSEngine *engine)
{
    if (QQmlEngine *qmlEngine = qobject_cast<QQmlEngine *>(engine))
        m_handler.addEngine(qmlEngine);
    emit attachedToEngine(engine);
}

void QQmlPreviewServiceImpl::engineAboutToBeRemoved(QJSEngine *engine)
{
    if (QQmlEngine *qmlEngine = qobject_cast<QQmlEngine *>(engine))
        m_handler.removeEngine(qmlEngine);
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

QQuickItem *QQmlPreviewServiceImpl::currentRootItem()
{
    return m_handler.currentRootItem();
}

QT_END_NAMESPACE

#include "moc_qqmlpreviewservice.cpp"
