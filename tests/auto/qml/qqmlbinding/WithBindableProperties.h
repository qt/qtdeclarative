// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef WithBindableProperties_H
#define WithBindableProperties_H

#include <QObject>
#include <qqml.h>
#include <QtCore/qproperty.h>


class WithBindableProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int a READ a WRITE setA BINDABLE bindableA)
    Q_PROPERTY(int b READ b WRITE setB BINDABLE bindableB)

public:
    QProperty<int> m_a;
    QProperty<int> m_b;
    int a() {return m_a;}
    int b() {return m_b;}
    void setA(int val) {m_a = val;}
    void setB(int val) {m_b = val;}
    QBindable<int> bindableA() {return QBindable<int>(&m_a); }
    QBindable<int> bindableB() {return QBindable<int>(&m_b); }
};

#endif

