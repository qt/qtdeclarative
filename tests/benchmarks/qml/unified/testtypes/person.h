// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QtQml/qqml.h>

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString nick READ nick WRITE setNick NOTIFY nickChanged FINAL)
    Q_PROPERTY(int shoeSize READ shoeSize WRITE setShoeSize)
    Q_PROPERTY(QVariantList things READ things WRITE setThings NOTIFY thingsChanged FINAL)
    QML_ELEMENT
public:
    Person(QObject *parent = nullptr);

    QString name() const;
    void setName(const QString &);

    QString nick() const;
    void setNick(const QString &);

    int shoeSize() const;
    void setShoeSize(int);

    QVariantList things() const;
    void setThings(const QVariantList &things);

signals:
    void nickChanged();
    void thingsChanged();

private:
    QVariantList m_things;
    QString m_name;
    QString m_nick;
    int m_shoeSize;
};

#endif // PERSON_H
