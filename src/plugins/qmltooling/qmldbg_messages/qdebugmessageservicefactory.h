// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QDEBUGMESSAGESERVICEFACTORY_H
#define QDEBUGMESSAGESERVICEFACTORY_H

#include <private/qqmldebugservicefactory_p.h>

QT_BEGIN_NAMESPACE

class QDebugMessageServiceFactory : public QQmlDebugServiceFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServiceFactory_iid FILE "qdebugmessageservice.json")
public:
    QQmlDebugService *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QDEBUGMESSAGESERVICEFACTORY_H
