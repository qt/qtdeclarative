// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef LOWERCASE_ENUM_H
#define LOWERCASE_ENUM_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

class LowercaseWithEnum : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(lowercaseType)

public:
    explicit LowercaseWithEnum(QObject *parent = nullptr) : QObject(parent) {}

    enum Status {
        Active,
        Inactive
    };
    Q_ENUM(Status)
};

#endif
// LOWERCASE_ENUM_H
