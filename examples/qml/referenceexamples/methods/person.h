// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QtQml/qqml.h>

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(int shoeSize READ shoeSize WRITE setShoeSize)
    QML_ELEMENT
public:
    using QObject::QObject;

    QString name() const;
    void setName(const QString &);

    int shoeSize() const;
    void setShoeSize(int);

private:
    QString m_name;
    int m_shoeSize = 0;
};

#endif // PERSON_H
