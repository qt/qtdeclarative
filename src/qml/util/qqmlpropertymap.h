// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLPROPERTYMAP_H
#define QQMLPROPERTYMAP_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlregistration.h>
#include <QtQml/qqmlprivate.h>

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE


class QQmlPropertyMapPrivate;
class Q_QML_EXPORT QQmlPropertyMap : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
#if QT_DEPRECATED_SINCE(6, 11)
    QT_DEPRECATED_VERSION_X_6_11("Use create() or the protected two-argument constructor instead.")
    explicit QQmlPropertyMap(QObject *parent = nullptr);
#endif

    static QQmlPropertyMap *create(QObject *parent = nullptr);

    ~QQmlPropertyMap() override;

    QVariant value(const QString &key) const;
    void insert(const QString &key, const QVariant &value);
    void insert(const QVariantHash &values);
    void clear(const QString &key);
    void freeze();

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
        Q_UNUSED(derived);
    }

private:
    friend class QtPrivate::QMetaTypeForType<QQmlPropertyMap>;

    QQmlPropertyMap(const QMetaObject *staticMetaObject, QObject *parent);

    Q_DECLARE_PRIVATE(QQmlPropertyMap)
    Q_DISABLE_COPY(QQmlPropertyMap)
};

namespace QtPrivate {
template<>
constexpr QMetaTypeInterface::DefaultCtrFn QMetaTypeForType<QQmlPropertyMap>::getDefaultCtr()
{
    return [](const QMetaTypeInterface *, void *addr) {
        new (addr) QQmlPropertyMap(&QQmlPropertyMap::staticMetaObject, nullptr);
    };
}
}

namespace QQmlPrivate {

// Specialization of QQmlElement for QQmlPropertyMap, for the rare case
// when you'd want to register QQmlPropertyMap directly, rather than some
// derived class of it.
template<>
inline QQmlElement<QQmlPropertyMap>::QQmlElement() : QQmlPropertyMap(this, nullptr) {}

}

QT_END_NAMESPACE

#endif
