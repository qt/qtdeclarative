// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QObject>
#include <QtQml>

class MyElement : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString someProp MEMBER m_someProp)
public:
    MyElement() = default;
    ~MyElement() = default;

private:
    QString m_someProp;
};
