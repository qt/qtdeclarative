// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef LOWERCASE_NAMESPACE_ENUM_H
#define LOWERCASE_NAMESPACE_ENUM_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

namespace lowercasenamespace {

class UppercaseWithEnum : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit UppercaseWithEnum(QObject *parent = nullptr) : QObject(parent) {}

    enum Status {
        Active,
        Inactive
    };
    Q_ENUM(Status)
};

} // namespace lowercasenamespace

#endif
// LOWERCASE_NAMESPACE_ENUM_H
