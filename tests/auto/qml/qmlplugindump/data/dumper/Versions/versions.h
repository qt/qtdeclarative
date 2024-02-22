// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef VERSIONS_H
#define VERSIONS_H

#include <QObject>

class Versions : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo NOTIFY fooChanged)
    Q_PROPERTY(int bar READ bar WRITE setBar NOTIFY barChanged REVISION 1)
    Q_PROPERTY(int baz READ baz WRITE setBaz NOTIFY bazChanged REVISION 2)

public:
    Versions(QObject *parent = nullptr);
    ~Versions();
    int foo() const { return m_foo; }
    void setFoo(int value) { m_foo = value; }
    int bar() const { return m_bar; }
    void setBar(int value) { m_bar = value; }
    int baz() const { return m_baz; }
    void setBaz(int value) { m_baz = value; }
signals:
    void fooChanged();
    void barChanged();
    void bazChanged();
private:
    int m_foo;
    int m_bar;
    int m_baz;
};

#endif // VERSIONS_H

