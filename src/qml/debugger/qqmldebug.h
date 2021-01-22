/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLDEBUG_H
#define QQMLDEBUG_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_debug)

struct Q_QML_EXPORT QQmlDebuggingEnabler
{
    enum StartMode {
        DoNotWaitForClient,
        WaitForClient
    };

    QQmlDebuggingEnabler(bool printWarning = true);

    static QStringList debuggerServices();
    static QStringList inspectorServices();
    static QStringList profilerServices();
    static QStringList nativeDebuggerServices();

    static void setServices(const QStringList &services);

    static bool startTcpDebugServer(int port, StartMode mode = DoNotWaitForClient,
                                    const QString &hostName = QString());
    static bool connectToLocalDebugger(const QString &socketFileName,
                                       StartMode mode = DoNotWaitForClient);
    static bool startDebugConnector(const QString &pluginName,
                                    const QVariantHash &configuration = QVariantHash());
};

// Execute code in constructor before first QQmlEngine is instantiated
#if defined(QT_QML_DEBUG_NO_WARNING)
static QQmlDebuggingEnabler qQmlEnableDebuggingHelper(false);
#elif defined(QT_QML_DEBUG)
static QQmlDebuggingEnabler qQmlEnableDebuggingHelper(true);
#endif

#endif

QT_END_NAMESPACE

#endif // QQMLDEBUG_H
