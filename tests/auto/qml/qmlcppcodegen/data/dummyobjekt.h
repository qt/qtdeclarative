// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DUMMYOBJEKT_H
#define DUMMYOBJEKT_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

#if QT_DEPRECATED_SINCE(6, 4)
class DummyObjekt : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Test {
        TestA = 1,
        TestB
    };
    Q_ENUM(Test)

    // Deliberately not default constructible
    explicit DummyObjekt(QObject *parent) : QObject(parent) {}
};
#endif

#endif // DUMMYOBJEKT_H
