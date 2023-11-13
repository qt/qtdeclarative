// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MYQMLELEMENTX_H
#define MYQMLELEMENTX_H

#include <QObject>
#include <QQmlEngine>

class MyQmlElementX : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit MyQmlElementX(QObject *parent = nullptr);

signals:

};

#endif // MYQMLELEMENTX_H
