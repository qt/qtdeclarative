// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DYNAMICMETA_H
#define DYNAMICMETA_H

#include <private/qobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <QtQmlIntegration/qqmlintegration.h>

template<typename T>
class MetaObjectData : public QDynamicMetaObjectData
{
    Q_DISABLE_COPY_MOVE(MetaObjectData)
public:
    MetaObjectData()
    {
        QMetaObjectBuilder builder;
        builder.setSuperClass(&T::staticMetaObject);
        builder.setFlags(builder.flags() | DynamicMetaObject);
        metaObject = builder.toMetaObject();
    };

    ~MetaObjectData() {
        free(metaObject);
    };

    QMetaObject *toDynamicMetaObject(QObject *) override
    {
        return metaObject;
    }
    int metaCall(QObject *o, QMetaObject::Call call, int idx, void **argv) override
    {
        return o->qt_metacall(call, idx, argv);
    }

    QMetaObject *metaObject = nullptr;
};

class DynamicMeta : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo NOTIFY fooChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue RESET resetValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal shadowable READ shadowable CONSTANT)
    QML_ELEMENT
public:

    DynamicMeta(QObject *parent = nullptr)
        : QObject(parent)
    {
        // deletes itself
        QObjectPrivate::get(this)->metaObject = new MetaObjectData<DynamicMeta>;
    }

    int foo() const { return m_foo; }
    void setFoo(int newFoo)
    {
        if (m_foo != newFoo) {
            m_foo = newFoo;
            emit fooChanged();
        }
    }

    Q_INVOKABLE int bar(int baz) { return baz + 12; }

    qreal value() const { return m_value; }
    qreal shadowable() const { return 25; }

public slots:
    void resetValue() { setValue(0); }
    void setValue(qreal value)
    {
        if (m_value == value)
            return;
        m_value = value;
        emit valueChanged();
    }

Q_SIGNALS:
    void fooChanged();
    void valueChanged();

private:
    int m_foo = 0;
    qreal m_value = 0;
};

class DynamicMetaSingleton : public DynamicMeta
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(DynamicMetaSingleton *itself READ itself CONSTANT FINAL)
public:
    DynamicMetaSingleton(QObject *parent = nullptr) : DynamicMeta(parent)
    {
        QObjectPrivate *d = QObjectPrivate::get(this);
        delete d->metaObject;
        d->metaObject = new MetaObjectData<DynamicMetaSingleton>;
    }

    DynamicMetaSingleton *itself() { return this; }
};

#endif // DYNAMICMETA_H
