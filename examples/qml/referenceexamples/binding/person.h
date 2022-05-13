// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QColor>
#include <QtQml/qqml.h>

class ShoeDescription : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY shoeChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY shoeChanged)
    Q_PROPERTY(QString brand READ brand WRITE setBrand NOTIFY shoeChanged)
    Q_PROPERTY(qreal price READ price WRITE setPrice NOTIFY shoeChanged)
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

signals:
    void shoeChanged();

private:
    int m_size = 0;
    QColor m_color;
    QString m_brand;
    qreal m_price = 0;
};

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
// ![0]
    Q_PROPERTY(ShoeDescription *shoe READ shoe CONSTANT)
// ![0]
    QML_ANONYMOUS
public:
    using QObject::QObject;

    QString name() const;
    void setName(const QString &);

    ShoeDescription *shoe();

signals:
    void nameChanged();

private:
    QString m_name;
    ShoeDescription m_shoe;
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
