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

#ifndef QLOCALCLIENTCONNECTIONFACTORY_H
#define QLOCALCLIENTCONNECTIONFACTORY_H

#include <private/qqmldebugserverconnection_p.h>

QT_BEGIN_NAMESPACE

class QLocalClientConnectionFactory : public QQmlDebugServerConnectionFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServerConnectionFactory_iid FILE "qlocalclientconnection.json")
    Q_INTERFACES(QQmlDebugServerConnectionFactory)
public:
    QQmlDebugServerConnection *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QLOCALCLIENTCONNECTIONFACTORY_H
