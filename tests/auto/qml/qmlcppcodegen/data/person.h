// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>

class Barzle : public QObject {};

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged RESET resetName FINAL)
    Q_PROPERTY(int shoeSize READ shoeSize WRITE setShoeSize NOTIFY shoeSizeChanged FINAL)
    Q_PROPERTY(int pain READ pain CONSTANT FINAL)
    Q_PROPERTY(QVariantList things READ things WRITE setThings NOTIFY thingsChanged FINAL)
    Q_PROPERTY(QList<Barzle *> barzles READ barzles WRITE setBarzles NOTIFY barzlesChanged FINAL)
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

signals:
    void nameChanged();
    void shoeSizeChanged();
    void thingsChanged();
    void barzlesChanged();

private:
    QString m_name;
    int m_shoeSize;
    QVariantList m_things;
    QList<Barzle *> m_barzles;
};

class FoozleListRegistration
{
    Q_GADGET
    QML_FOREIGN(QList<Barzle *>)
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(Barzle *)
};


#endif // PERSON_H
