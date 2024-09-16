// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef TYPES_H
#define TYPES_H

#include <qqmlregistration.h>
#include <QObject>

class Test : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 1)
    QML_EXTRA_VERSION(2, 0)
    QML_EXTRA_VERSION(3, 0)

public:
    Q_INVOKABLE bool check() const;
};


#endif
