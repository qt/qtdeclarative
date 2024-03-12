// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtCore/qproperty.h>
#include <QtCore/qrect.h>

struct Inner
{
    Q_GADGET
    QML_VALUE_TYPE(inner)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(int i MEMBER i)

private:
    friend bool operator==(const Inner &lhs, const Inner &rhs) { return lhs.i == rhs.i; }
    friend bool operator!=(const Inner &lhs, const Inner &rhs) { return !(lhs == rhs); }

    int i = 11;
};

struct Outer
{
    Q_GADGET
    QML_VALUE_TYPE(outer)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(Inner inner MEMBER inner)

private:
    friend bool operator==(const Outer &lhs, const Outer &rhs) { return lhs.inner == rhs.inner; }
    friend bool operator!=(const Outer &lhs, const Outer &rhs) { return !(lhs == rhs); }

    Inner inner;
};

// Intentionally opaque type
class Barzle : public QObject {};

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged RESET resetName FINAL)
    Q_PROPERTY(int shoeSize READ shoeSize WRITE setShoeSize NOTIFY shoeSizeChanged FINAL)
    Q_PROPERTY(int pain READ pain CONSTANT FINAL)
    Q_PROPERTY(QVariantList things READ things WRITE setThings NOTIFY thingsChanged FINAL)
    Q_PROPERTY(QList<Barzle *> barzles READ barzles WRITE setBarzles NOTIFY barzlesChanged FINAL)
    Q_PROPERTY(QList<Person *> cousins READ cousins WRITE setCousins NOTIFY cousinsChanged FINAL)
    Q_PROPERTY(QByteArray data READ data WRITE setData NOTIFY dataChanged FINAL)
    Q_PROPERTY(QRectF area READ area WRITE setArea NOTIFY areaChanged) // not FINAL
    Q_PROPERTY(QRectF area2 READ area WRITE setArea NOTIFY areaChanged BINDABLE areaBindable FINAL)
    QML_ELEMENT
public:
    Person(QObject *parent = nullptr);
    ~Person() { setShoeSize(-25); }

    int pain() const {
        qmlEngine(this)->throwError(QStringLiteral("ouch"));
        return 92;
    }

    QString name() const;
    void setName(const QString &);
    void resetName();

    int shoeSize() const;
    void setShoeSize(int);

    QVariantList things() const;
    void setThings(const QVariantList &things);

    QList<Barzle *> barzles() const;
    void setBarzles(const QList<Barzle *> &foozles);

    QBindable<QByteArray> dataBindable();
    void setData(const QByteArray &data);
    QByteArray data() const;

    QList<Person *> cousins() const;
    void setCousins(const QList<Person *> &newCousins);

    QRectF area() const;
    void setArea(const QRectF &newArea);
    QBindable<QRectF> areaBindable();

    Q_INVOKABLE QString getName() const { return m_name; }

signals:
    void nameChanged();
    void shoeSizeChanged();
    void thingsChanged();
    void barzlesChanged();
    void dataChanged();

    void ambiguous(int a = 9);

    void cousinsChanged();
    void objectListHappened(const QList<QObject *> &);
    void variantListHappened(const QList<QVariant> &);

    void areaChanged();

private:
    QString m_name;
    int m_shoeSize;
    QVariantList m_things;
    QList<Barzle *> m_barzles;
    QList<Person *> m_cousins;
    QProperty<QByteArray> m_data;
    QProperty<QRectF> m_area;
};

class BarzleListRegistration
{
    Q_GADGET
    QML_FOREIGN(QList<Barzle *>)
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(Barzle *)
};

class PersonListRegistration
{
    Q_GADGET
    QML_FOREIGN(QList<Person *>)
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(Person *)
};

#endif // PERSON_H
