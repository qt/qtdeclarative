// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SHADOWED_H
#define SHADOWED_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

// There is also a Shadowed.qml in the same module. The engine prefers this type.
class Shadowed : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString origin READ origin CONSTANT)

public:
    Shadowed(QObject *parent = nullptr) : QObject(parent) {}

    QString origin() const { return QString::fromLatin1("cpp"); }
};

#endif // SHADOWED_H
