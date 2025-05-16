// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
