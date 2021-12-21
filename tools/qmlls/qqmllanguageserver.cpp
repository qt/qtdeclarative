/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qqmllanguageserver.h"

#include "textsynchronization.h"

#include "qlanguageserver.h"
#include <QtCore/qdir.h>

#include <iostream>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

using namespace QLspSpecification;
/*!
\internal
\class QmlLsp::QQmlLanguageServer
\brief Class that sets up a QmlLanguageServer

This class sets up a QML language server.
It needs a function
\code
std::function<void(const QByteArray &)> sendData
\endcode
to send out its replies, and one should feed the data it receives to the server()->receive() method.
It is expected to call this method only from a single thread, and not to block, the simplest way to
achieve this is to avoid direct calls, and connect it as slot, while reading from another thread.

The Server is build with separate QLanguageServerModule that implement a given functionality, and
all of them are constructed and registered with the QLanguageServer in the constructor o this class.

Generally all operations are expected to be done in the object thread, and handlers are always
called from it, but they are free to delegate the response to another thread, the response handler
is thread safe. All the methods of the server() obect are also threadsafe.

The code model starts other threads to update its state, see its documentation for more information.
*/
QQmlLanguageServer::QQmlLanguageServer(std::function<void(const QByteArray &)> sendData)
    : m_server(sendData),
      m_textSynchronization(&m_codeModel),
      m_lint(&m_server, &m_codeModel)
{
    m_server.addServerModule(this);
    m_server.addServerModule(&m_textSynchronization);
    m_server.finishSetup();
    qCWarning(lspServerLog) << "Did Setup";
}

void QQmlLanguageServer::registerHandlers(QLanguageServer *server,
                                          QLanguageServerProtocol *protocol)
{
    Q_UNUSED(protocol);
    QObject::connect(server, &QLanguageServer::lifecycleError, this,
                     &QQmlLanguageServer::errorExit);
    QObject::connect(server, &QLanguageServer::exit, this, &QQmlLanguageServer::exit);
    QObject::connect(server, &QLanguageServer::runStatusChanged, [](QLanguageServer::RunStatus r) {
        qCDebug(lspServerLog) << "runStatus" << int(r);
    });
}

void QQmlLanguageServer::setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                                           QLspSpecification::InitializeResult &serverInfo)
{
    Q_UNUSED(clientInfo);
    Q_UNUSED(serverInfo);
}

QString QQmlLanguageServer::name() const
{
    return u"QQmlLanguageServer"_qs;
}

void QQmlLanguageServer::errorExit()
{
    qCWarning(lspServerLog) << "Error exit";
    fclose(stdin);
}

void QQmlLanguageServer::exit()
{
    m_returnValue = 0;
    fclose(stdin);
}

int QQmlLanguageServer::returnValue() const
{
    return m_returnValue;
}

QLanguageServer *QQmlLanguageServer::server()
{
    return &m_server;
}

TextSynchronization *QQmlLanguageServer::textSynchronization()
{
    return &m_textSynchronization;
}

QmlLintSuggestions *QQmlLanguageServer::lint()
{
    return &m_lint;
}

} // namespace QmlLsp

QT_END_NAMESPACE
