/******************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt JavaScript to C++ compiler.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged RESET resetName FINAL)
    Q_PROPERTY(int shoeSize READ shoeSize WRITE setShoeSize NOTIFY shoeSizeChanged FINAL)
    Q_PROPERTY(int pain READ pain CONSTANT FINAL)
    Q_PROPERTY(QVariantList things READ things WRITE setThings NOTIFY thingsChanged FINAL)
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

signals:
    void nameChanged();
    void shoeSizeChanged();
    void thingsChanged();

private:
    QString m_name;
    int m_shoeSize;
    QVariantList m_things;
};

#endif // PERSON_H
