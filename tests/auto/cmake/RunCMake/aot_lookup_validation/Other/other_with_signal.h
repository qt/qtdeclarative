// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class Other : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Q_INVOKABLE void f() {}

signals:
    void s();
};
