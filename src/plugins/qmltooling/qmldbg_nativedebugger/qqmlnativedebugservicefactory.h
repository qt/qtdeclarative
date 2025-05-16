// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQMLNATIVEDEBUGSERVICEFACTORY_H
#define QQMLNATIVEDEBUGSERVICEFACTORY_H

#include <private/qqmldebugservicefactory_p.h>

QT_BEGIN_NAMESPACE

class QQmlNativeDebugServiceFactory : public QQmlDebugServiceFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServiceFactory_iid FILE "qqmlnativedebugservice.json")
public:
    QQmlDebugService *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QQMLNATIVEDEBUGSERVICEFACTORY_H
