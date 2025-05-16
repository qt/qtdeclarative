// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQMLDEBUGGERSERVICEFACTORY_H
#define QQMLDEBUGGERSERVICEFACTORY_H

#include <private/qqmldebugservicefactory_p.h>

QT_BEGIN_NAMESPACE

class QQmlDebuggerServiceFactory : public QQmlDebugServiceFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServiceFactory_iid FILE "qqmldebuggerservice.json")
public:
    QQmlDebugService *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGGERSERVICEFACTORY_H
