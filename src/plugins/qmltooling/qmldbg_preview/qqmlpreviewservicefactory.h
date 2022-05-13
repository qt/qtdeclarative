// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLPREVIEWSERVCIEFACTORY_H
#define QQMLPREVIEWSERVCIEFACTORY_H

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

#include <private/qqmldebugservicefactory_p.h>

QT_BEGIN_NAMESPACE

class QQmlPreviewServiceFactory : public QQmlDebugServiceFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServiceFactory_iid FILE "qqmlpreviewservice.json")

public:
    QQmlDebugService *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWSERVCIEFACTORY_H
