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

#ifndef QQMLDEBUGSERVERCONNECTION_P_H
#define QQMLDEBUGSERVERCONNECTION_P_H

#include <private/qtqmlglobal_p.h>
#include <QtCore/qobject.h>

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

QT_BEGIN_NAMESPACE

class QQmlDebugServer;
class Q_QML_PRIVATE_EXPORT QQmlDebugServerConnection : public QObject
{
    Q_OBJECT
public:
    QQmlDebugServerConnection(QObject *parent = nullptr) : QObject(parent) {}

    virtual void setServer(QQmlDebugServer *server) = 0;
    virtual bool setPortRange(int portFrom, int portTo, bool block, const QString &hostaddress) = 0;
    virtual bool setFileName(const QString &fileName, bool block) = 0;
    virtual bool isConnected() const = 0;
    virtual void disconnect() = 0;
    virtual void waitForConnection() = 0;
    virtual void flush() = 0;
};

class Q_QML_PRIVATE_EXPORT QQmlDebugServerConnectionFactory : public QObject
{
    Q_OBJECT
public:
    virtual QQmlDebugServerConnection *create(const QString &key) = 0;
};

#define QQmlDebugServerConnectionFactory_iid "org.qt-project.Qt.QQmlDebugServerConnectionFactory"
Q_DECLARE_INTERFACE(QQmlDebugServerConnectionFactory, QQmlDebugServerConnectionFactory_iid)

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVERCONNECTION_H
