/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLPROPERTYMAP_H
#define QQMLPROPERTYMAP_H

#include <QtQml/qtqmlglobal.h>

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE


class QQmlPropertyMapPrivate;
class Q_QML_EXPORT QQmlPropertyMap : public QObject
{
    Q_OBJECT
public:
    explicit QQmlPropertyMap(QObject *parent = nullptr);
    ~QQmlPropertyMap() override;

    QVariant value(const QString &key) const;
    void insert(const QString &key, const QVariant &value);
    void clear(const QString &key);

    Q_INVOKABLE QStringList keys() const;

    int count() const;
    int size() const;
    bool isEmpty() const;
    bool contains(const QString &key) const;

    QVariant &operator[](const QString &key);
    QVariant operator[](const QString &key) const;

Q_SIGNALS:
    void valueChanged(const QString &key, const QVariant &value);

protected:
    virtual QVariant updateValue(const QString &key, const QVariant &input);

    template<class DerivedType>
    QQmlPropertyMap(DerivedType *derived, QObject *parentObj)
        : QQmlPropertyMap(&DerivedType::staticMetaObject, parentObj)
    {
        Q_UNUSED(derived)
    }

private:
    QQmlPropertyMap(const QMetaObject *staticMetaObject, QObject *parent);

    Q_DECLARE_PRIVATE(QQmlPropertyMap)
    Q_DISABLE_COPY(QQmlPropertyMap)
};

QT_END_NAMESPACE

#endif
