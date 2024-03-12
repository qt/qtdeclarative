// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class MyQmlObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int result READ result WRITE setResult)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(MyQmlObject *object READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data)
    Q_CLASSINFO("DefaultProperty", "data")
public:
    MyQmlObject() : m_result(0), m_value(0), m_object(0) {}

    int result() const { return m_result; }
    void setResult(int r) { m_result = r; }

    int value() const { return m_value; }
    void setValue(int v) { m_value = v; emit valueChanged(); }

    QQmlListProperty<QObject> data() { return QQmlListProperty<QObject>(this, &m_data); }

    MyQmlObject *object() const { return m_object; }
    void setObject(MyQmlObject *o) { m_object = o; emit objectChanged(); }

signals:
    void valueChanged();
    void objectChanged();

private:
    QList<QObject *> m_data;
    int m_result;
    int m_value;
    MyQmlObject *m_object;
};
QML_DECLARE_TYPE(MyQmlObject);

void registerTypes();

#endif // TESTTYPES_H
