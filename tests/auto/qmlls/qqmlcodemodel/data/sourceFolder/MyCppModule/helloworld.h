// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include <QObject>
#include <QQmlEngine>

class HelloWorld : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int myP READ myP WRITE setMyP NOTIFY myPChanged FINAL)
    Q_PROPERTY(int myPPP READ myP WRITE setMyP NOTIFY myPChanged FINAL)

public:
    explicit HelloWorld(QObject *parent = nullptr);

    int myP() { return m_myP; }
    void setMyP(int p) { m_myP = p; }
private:
    int m_myP;

signals:
    void myPChanged();
};

#endif // HELLOWORLD_H
