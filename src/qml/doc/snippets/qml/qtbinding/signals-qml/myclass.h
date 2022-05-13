// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QObject>
#include <QDebug>
//![0]
class MyClass : public QObject
{
    Q_OBJECT
public slots:
    void cppSlot(const QString &msg) {
        qDebug() << "Called the C++ slot with message:" << msg;
    }
};
//![0]
