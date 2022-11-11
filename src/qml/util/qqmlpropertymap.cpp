// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertymap.h"

#include <private/qmetaobjectbuilder_p.h>
#include <private/qqmlopenmetaobject_p.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

//QQmlPropertyMapMetaObject lets us listen for changes coming from QML
//so we can emit the changed signal.
class QQmlPropertyMapMetaObject : public QQmlOpenMetaObject
{
public:
    QQmlPropertyMapMetaObject(QQmlPropertyMap *obj, QQmlPropertyMapPrivate *objPriv, const QMetaObject *staticMetaObject);

protected:
    QVariant propertyWriteValue(int, const QVariant &) override;
    void propertyWritten(int index) override;
    void propertyCreated(int, QMetaPropertyBuilder &) override;

    const QString &propertyName(int index);

private:
    QQmlPropertyMap *map;
    QQmlPropertyMapPrivate *priv;
};

class QQmlPropertyMapPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlPropertyMap)
public:
    QQmlPropertyMapMetaObject *mo;
    QStringList keys;

    QVariant updateValue(const QString &key, const QVariant &input);
    void emitChanged(const QString &key, const QVariant &value);
    bool validKeyName(const QString& name);

    const QString &propertyName(int index) const;
};

bool QQmlPropertyMapPrivate::validKeyName(const QString& name)
{
    //The following strings shouldn't be used as property names
    return  name != QLatin1String("keys")
         && name != QLatin1String("valueChanged")
         && name != QLatin1String("QObject")
         && name != QLatin1String("destroyed")
         && name != QLatin1String("deleteLater");
}

QVariant QQmlPropertyMapPrivate::updateValue(const QString &key, const QVariant &input)
{
    Q_Q(QQmlPropertyMap);
    return q->updateValue(key, input);
}

void QQmlPropertyMapPrivate::emitChanged(const QString &key, const QVariant &value)
{
    Q_Q(QQmlPropertyMap);
    emit q->valueChanged(key, value);
}

const QString &QQmlPropertyMapPrivate::propertyName(int index) const
{
    Q_ASSERT(index < keys.size());
    return keys[index];
}

QQmlPropertyMapMetaObject::QQmlPropertyMapMetaObject(QQmlPropertyMap *obj, QQmlPropertyMapPrivate *objPriv, const QMetaObject *staticMetaObject)
    : QQmlOpenMetaObject(obj, staticMetaObject)
{
    map = obj;
    priv = objPriv;
}

QVariant QQmlPropertyMapMetaObject::propertyWriteValue(int index, const QVariant &input)
{
    return priv->updateValue(priv->propertyName(index), input);
}

void QQmlPropertyMapMetaObject::propertyWritten(int index)
{
    priv->emitChanged(priv->propertyName(index), value(index));
}

void QQmlPropertyMapMetaObject::propertyCreated(int, QMetaPropertyBuilder &b)
{
    priv->keys.append(QString::fromUtf8(b.name()));
}

/*!
    \class QQmlPropertyMap
    \brief The QQmlPropertyMap class allows you to set key-value pairs that can be used in QML bindings.
    \inmodule QtQml

    QQmlPropertyMap provides a convenient way to expose domain data to the UI layer.
    The following example shows how you might declare data in C++ and then
    access it in QML.

    In the C++ file:
    \code
    // create our data
    QQmlPropertyMap ownerData;
    ownerData.insert("name", QVariant(QString("John Smith")));
    ownerData.insert("phone", QVariant(QString("555-5555")));

    // expose it to the UI layer
    QQuickView view;
    QQmlContext *ctxt = view.rootContext();
    ctxt->setContextProperty("owner", &ownerData);

    view.setSource(QUrl::fromLocalFile("main.qml"));
    view.show();
    \endcode

    Then, in \c main.qml:
    \code
    Text { text: owner.name + " " + owner.phone }
    \endcode

    The binding is dynamic - whenever a key's value is updated, anything bound to that
    key will be updated as well.

    To detect value changes made in the UI layer you can connect to the valueChanged() signal.
    However, note that valueChanged() is \b NOT emitted when changes are made by calling insert()
    or clear() - it is only emitted when a value is updated from QML.

    \note It is not possible to remove keys from the map; once a key has been added, you can only
    modify or clear its associated value.

    \note When deriving a class from QQmlPropertyMap, use the
    \l {QQmlPropertyMap::QQmlPropertyMap(DerivedType *derived, QObject *parent)} {protected two-argument constructor}
    which ensures that the class is correctly registered with the Qt \l {Meta-Object System}.

    \note The QMetaObject of a QQmlPropertyMap is dynamically generated and modified.
    Operations on that meta object are not thread safe, so applications need to take
    care to explicitly synchronize access to the meta object.
*/

/*!
    Constructs a bindable map with parent object \a parent.
*/
QQmlPropertyMap::QQmlPropertyMap(QObject *parent)
: QQmlPropertyMap(&staticMetaObject, parent)
{
}

/*!
    Destroys the bindable map.
*/
QQmlPropertyMap::~QQmlPropertyMap()
{
}

/*!
    Clears the value (if any) associated with \a key.
*/
void QQmlPropertyMap::clear(const QString &key)
{
    Q_D(QQmlPropertyMap);
    if (d->validKeyName(key))
        d->mo->setValue(key.toUtf8(), QVariant());
}

/*!
    \since 6.1

    Disallows any further properties to be added to this property map.
    Existing properties can be modified or cleared.

    In turn, an internal cache is turned on for the existing properties, which
    may result in faster access from QML.
 */
void QQmlPropertyMap::freeze()
{
    Q_D(QQmlPropertyMap);
    d->mo->setAutoCreatesProperties(false);
    d->mo->setCached(true);
}

/*!
    Returns the value associated with \a key.

    If no value has been set for this key (or if the value has been cleared),
    an invalid QVariant is returned.
*/
QVariant QQmlPropertyMap::value(const QString &key) const
{
    Q_D(const QQmlPropertyMap);
    return d->mo->value(key.toUtf8());
}

/*!
    Sets the value associated with \a key to \a value.

    If the key doesn't exist, it is automatically created.
*/
void QQmlPropertyMap::insert(const QString &key, const QVariant &value)
{
    Q_D(QQmlPropertyMap);

    if (d->validKeyName(key)) {
        d->mo->setValue(key.toUtf8(), value);
    } else {
        qWarning() << "Creating property with name"
                   << key
                   << "is not permitted, conflicts with internal symbols.";
    }
}

/*!
    \since 6.1

    Inserts the \a values into the QQmlPropertyMap.

    Keys that don't exist are automatically created.

    This method is substantially faster than calling \c{insert(key, value)}
    many times in a row.
*/
void QQmlPropertyMap::insert(const QVariantHash &values)
{
    Q_D(QQmlPropertyMap);

    QHash<QByteArray, QVariant> checkedValues;
    for (auto it = values.begin(), end = values.end(); it != end; ++it) {
        const QString &key = it.key();
        if (!d->validKeyName(key)) {
            qWarning() << "Creating property with name"
                       << key
                       << "is not permitted, conflicts with internal symbols.";
            return;
        }

        checkedValues.insert(key.toUtf8(), it.value());
    }
    d->mo->setValues(checkedValues);

}

/*!
    Returns the list of keys.

    Keys that have been cleared will still appear in this list, even though their
    associated values are invalid QVariants.
*/
QStringList QQmlPropertyMap::keys() const
{
    Q_D(const QQmlPropertyMap);
    return d->keys;
}

/*!
    \overload

    Same as size().
*/
int QQmlPropertyMap::count() const
{
    Q_D(const QQmlPropertyMap);
    return d->keys.size();
}

/*!
    Returns the number of keys in the map.

    \sa isEmpty(), count()
*/
int QQmlPropertyMap::size() const
{
    Q_D(const QQmlPropertyMap);
    return d->keys.size();
}

/*!
    Returns true if the map contains no keys; otherwise returns
    false.

    \sa size()
*/
bool QQmlPropertyMap::isEmpty() const
{
    Q_D(const QQmlPropertyMap);
    return d->keys.isEmpty();
}

/*!
    Returns true if the map contains \a key.

    \sa size()
*/
bool QQmlPropertyMap::contains(const QString &key) const
{
    Q_D(const QQmlPropertyMap);
    return d->keys.contains(key);
}

/*!
    Returns the value associated with the key \a key as a modifiable
    reference.

    If the map contains no item with key \a key, the function inserts
    an invalid QVariant into the map with key \a key, and
    returns a reference to it.

    \sa insert(), value()
*/
QVariant &QQmlPropertyMap::operator[](const QString &key)
{
    //### optimize
    Q_D(QQmlPropertyMap);
    QByteArray utf8key = key.toUtf8();
    if (!d->keys.contains(key))
        insert(key, QVariant());//force creation -- needed below

    return d->mo->valueRef(utf8key);
}

/*!
    \overload

    Same as value().
*/
QVariant QQmlPropertyMap::operator[](const QString &key) const
{
    return value(key);
}

/*!
    Returns the new value to be stored for the key \a key.  This function is provided
    to intercept updates to a property from QML, where the value provided from QML is \a input.

    Override this function to manipulate the property value as it is updated.  Note that
    this function is only invoked when the value is updated from QML.
*/
QVariant QQmlPropertyMap::updateValue(const QString &key, const QVariant &input)
{
    Q_UNUSED(key);
    return input;
}

/*! \internal */
QQmlPropertyMap::QQmlPropertyMap(const QMetaObject *staticMetaObject, QObject *parent)
    : QObject(*(new QQmlPropertyMapPrivate), parent)
{
    Q_D(QQmlPropertyMap);
    d->mo = new QQmlPropertyMapMetaObject(this, d, staticMetaObject);
}

/*!
    \fn void QQmlPropertyMap::valueChanged(const QString &key, const QVariant &value)
    This signal is emitted whenever one of the values in the map is changed. \a key
    is the key corresponding to the \a value that was changed.

    \note valueChanged() is \b NOT emitted when changes are made by calling insert()
    or clear() - it is only emitted when a value is updated from QML.
*/

/*!
    \fn template<class DerivedType> QQmlPropertyMap::QQmlPropertyMap(DerivedType *derived, QObject *parent)

    Constructs a bindable map with parent object \a parent.  Use this constructor
    in classes derived from QQmlPropertyMap.

    The type of \a derived is used to register the property map with the \l {Meta-Object System},
    which is necessary to ensure that properties of the derived class are accessible.
    This type must be derived from QQmlPropertyMap.
*/

QT_END_NAMESPACE

#include "moc_qqmlpropertymap.cpp"
