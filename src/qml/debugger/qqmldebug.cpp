/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmldebug.h"
#include "qqmldebugconnector_p.h"

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QQmlDebuggingEnabler::QQmlDebuggingEnabler(bool printWarning)
{
#ifndef QQML_NO_DEBUG_PROTOCOL
    if (!QQmlEnginePrivate::qml_debugging_enabled
            && printWarning) {
        qDebug("QML debugging is enabled. Only use this in a safe environment.");
    }
    QQmlEnginePrivate::qml_debugging_enabled = true;
#else
    Q_UNUSED(printWarning);
#endif
}

/*!
 * \enum QQmlDebuggingEnabler::StartMode
 *
 * Defines the debug server's start behavior. You can interrupt QML engines starting while a debug
 * client is connecting, in order to set breakpoints in or profile startup code.
 *
 * \value DoNotWaitForClient Run any QML engines as usual while the debug services are connecting.
 * \value WaitForClient      If a QML engine starts while the debug services are connecting,
 *                           interrupt it until they are done.
 */

/*!
 * Enables debugging for QML engines created after calling this function. The debug server will
 * listen on \a port at \a hostName and block the QML engine until it receives a connection if
 * \a mode is \c WaitForClient. If \a mode is not specified it won't block and if \a hostName is not
 * specified it will listen on all available interfaces. You can only start one debug server at a
 * time. A debug server may have already been started if the -qmljsdebugger= command line argument
 * was given. This method returns \c true if a new debug server was successfully started, or
 * \c false otherwise.
 */
bool QQmlDebuggingEnabler::startTcpDebugServer(int port, StartMode mode, const QString &hostName)
{
#ifndef QQML_NO_DEBUG_PROTOCOL
    QQmlDebugConnector::setPluginKey(QLatin1String("QQmlDebugServer"));
    QQmlDebugConnector *connector = QQmlDebugConnector::instance();
    if (connector) {
        QVariantHash configuration;
        configuration[QLatin1String("portFrom")] = configuration[QLatin1String("portTo")] = port;
        configuration[QLatin1String("block")] = (mode == WaitForClient);
        configuration[QLatin1String("hostAddress")] = hostName;
        return connector->open(configuration);
    }
#else
    Q_UNUSED(port);
    Q_UNUSED(block);
    Q_UNUSED(hostName);
#endif
    return false;
}

/*!
 * \since 5.6
 *
 * Enables debugging for QML engines created after calling this function. The debug server will
 * connect to a debugger waiting on a local socket at the given \a socketFileName and block the QML
 * engine until the connection is established if \a mode is \c WaitForClient. If \a mode is not
 * specified it will not block. You can only start one debug server at a time. A debug server may
 * have already been started if the -qmljsdebugger= command line argument was given. This method
 * returns \c true if a new debug server was successfully started, or \c false otherwise.
 */
bool QQmlDebuggingEnabler::connectToLocalDebugger(const QString &socketFileName, StartMode mode)
{
#ifndef QQML_NO_DEBUG_PROTOCOL
    QQmlDebugConnector::setPluginKey(QLatin1String("QQmlDebugServer"));
    QQmlDebugConnector *connector = QQmlDebugConnector::instance();
    if (connector) {
        QVariantHash configuration;
        configuration[QLatin1String("fileName")] = socketFileName;
        configuration[QLatin1String("block")] = (mode == WaitForClient);
        return connector->open(configuration);
    }
#else
    Q_UNUSED(fileName);
    Q_UNUSED(block);
#endif
    return false;
}

enum { HookCount = 3 };

// Only add to the end, and bump version if you do.
quintptr Q_QML_EXPORT qtDeclarativeHookData[] = {
    // Version of this Array. Bump if you add to end.
    1,

    // Number of entries in this array.
    HookCount,

    // TypeInformationVersion, an integral value, bumped whenever private
    // object sizes or member offsets that are used in Qt Creator's
    // data structure "pretty printing" change.
    2
};

Q_STATIC_ASSERT(HookCount == sizeof(qtDeclarativeHookData) / sizeof(qtDeclarativeHookData[0]));

QT_END_NAMESPACE
