// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqml.h>

class Attached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int a READ a WRITE setA NOTIFY aChanged)
    QML_ELEMENT
    QML_ATTACHED(Attached)
    QML_UNCREATABLE("")

public:
    Attached(QObject *parent = nullptr);

    int a() const;
    void setA(int a);

    static Attached *qmlAttachedProperties(QObject *object);

signals:
    void aChanged();
    void stuffChanged();

private:
    int mA = 0;
};
