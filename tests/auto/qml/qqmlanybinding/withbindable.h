// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef WITH_BINDABLE_H
#define WITH_BINDABLE_H

#include <QObject>
#include <QtCore/qproperty.h>
#include <qqml.h>

class WithBinding : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int prop READ prop WRITE setProp BINDABLE bindableProp)
public:
    int prop() {return m_prop;}
    void setProp(int i) {m_prop = i;}
    QBindable<int> bindableProp() {return &m_prop;}
private:
    QProperty<int> m_prop;
};

#endif

