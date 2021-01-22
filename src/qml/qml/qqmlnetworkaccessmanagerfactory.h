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

#ifndef QQMLNETWORKACCESSMANAGERFACTORY_H
#define QQMLNETWORKACCESSMANAGERFACTORY_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_network)

class QNetworkAccessManager;
class Q_QML_EXPORT QQmlNetworkAccessManagerFactory
{
public:
    virtual ~QQmlNetworkAccessManagerFactory();
    virtual QNetworkAccessManager *create(QObject *parent) = 0;

};

#endif // qml_network

QT_END_NAMESPACE

#endif // QQMLNETWORKACCESSMANAGERFACTORY_H
