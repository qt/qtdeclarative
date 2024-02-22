// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ENEMY_H
#define ENEMY_H

#include <QtQml/qqml.h>
#include <QtCore/qproperty.h>

class Enemy : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Enemy)
    Q_PROPERTY(QString name MEMBER name BINDABLE bindableName)

public:
    Enemy(QObject *parent = nullptr);

    QProperty<QString> name;
    QBindable<QString> bindableName() { return QBindable<QString>(&name); }
};

#endif // ENEMY_H
