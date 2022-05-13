// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDEBUGSERVERFACTORY_H
#define QQMLDEBUGSERVERFACTORY_H

#include <private/qqmldebugconnector_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// ementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQmlDebugServerFactory : public QQmlDebugConnectorFactory
{
    Q_OBJECT

    // The interface for the code that loads this thing is QQmlDebugConnector.
    // QQmlDebugServer is for connection plugins.
    Q_PLUGIN_METADATA(IID QQmlDebugConnectorFactory_iid FILE "qqmldebugserver.json")
public:
    QQmlDebugConnector *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVERFACTORY_H
