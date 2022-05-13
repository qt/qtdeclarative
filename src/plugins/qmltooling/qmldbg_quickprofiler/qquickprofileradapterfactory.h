// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKPROFILERADAPTERFACTORY_H
#define QQUICKPROFILERADAPTERFACTORY_H

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

#include <QtQml/private/qqmlabstractprofileradapter_p.h>
#include <QtQuick/private/qquickprofiler_p.h>

QT_BEGIN_NAMESPACE

class QQuickProfilerAdapterFactory : public QQmlAbstractProfilerAdapterFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlAbstractProfilerAdapterFactory_iid FILE "qquickprofileradapter.json")
public:
    QQmlAbstractProfilerAdapter *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QQUICKPROFILERADAPTERFACTORY_H
