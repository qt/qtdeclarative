// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PERSON_H
#define PERSON_H

#include <QtQml/qqml.h>
#include <QObject>
#include <QColor>

class ShoeDescription : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY shoeChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY shoeChanged FINAL)
    Q_PROPERTY(QString brand READ brand WRITE setBrand NOTIFY shoeChanged FINAL)
    Q_PROPERTY(qreal price READ price WRITE setPrice NOTIFY shoeChanged FINAL)
    QML_ANONYMOUS
public:
    using QObject::QObject;

    int size() const;
    void setSize(int);

    QColor color() const;
    void setColor(const QColor &);

    QString brand() const;
    void setBrand(const QString &);

    qreal price() const;
    void setPrice(qreal);

    friend bool operator==(const ShoeDescription &lhs, const ShoeDescription &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }
    friend bool operator!=(const ShoeDescription &lhs, const ShoeDescription &rhs)
    {
        return !operatorEqualsImpl(lhs, rhs);
    }

signals:
    void shoeChanged();

private:
    static bool operatorEqualsImpl(const ShoeDescription &, const ShoeDescription &);

    int m_size = 0;
    QColor m_color;
    QString m_brand;
    qreal m_price = 0;
};

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(ShoeDescription *shoe READ shoe WRITE setShoe NOTIFY shoeChanged FINAL)
    QML_ELEMENT
    QML_UNCREATABLE("Person is an abstract base class.")
public:
    using QObject::QObject;

    Person(QObject *parent = nullptr);

    QString name() const;
    void setName(const QString &);

    ShoeDescription *shoe() const;
    void setShoe(ShoeDescription *shoe);

signals:
    void nameChanged();
    void shoeChanged();

private:
    QString m_name;
    ShoeDescription *m_shoe = nullptr;
};

class Boy : public Person
{
    Q_OBJECT
    QML_ELEMENT
public:
    using Person::Person;
};

class Girl : public Person
{
    Q_OBJECT
    QML_ELEMENT
public:
    using Person::Person;
};

#endif // PERSON_H
