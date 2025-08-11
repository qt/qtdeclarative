// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QtQml/qjsengine.h>

class Collector : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int gc READ gc CONSTANT)

public:
    int gc() const
    {
        qjsEngine(this)->collectGarbage();
        return 1;
    }
};

#endif // COLLECTOR_H
