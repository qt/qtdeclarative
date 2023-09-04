// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlproperty.h"
#include "qqmlproperty_p.h"

#include "qqmlboundsignal_p.h"
#include "qqmlcontext.h"
#include "qqmlboundsignal_p.h"
#include "qqmlengine.h"
#include "qqmlengine_p.h"
#include "qqmldata_p.h"
#include "qqmlstringconverters_p.h"

#include "qqmlvmemetaobject_p.h"
#include "qqmlvaluetypeproxybinding_p.h"
#include <private/qjsvalue_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmlirbuilder_p.h>
#include <QtQml/private/qqmllist_p.h>

#include <QStringList>
#include <QVector>
#include <private/qmetaobject_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <QtCore/qdebug.h>
#include <cmath>
#include <QtQml/QQmlPropertyMap>
#include <QtCore/private/qproperty_p.h>
#include <QtCore/qsequentialiterable.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(compatResolveUrlsOnAssigment, QML_COMPAT_RESOLVE_URLS_ON_ASSIGNMENT);

/*!
\class QQmlProperty
\since 5.0
\inmodule QtQml
\brief The QQmlProperty class abstracts accessing properties on objects created from  QML.

As QML uses Qt's meta-type system all of the existing QMetaObject classes can be used to introspect
and interact with objects created by QML.  However, some of the new features provided by QML - such
as type safety and attached properties - are most easily used through the QQmlProperty class
that simplifies some of their natural complexity.

Unlike QMetaProperty which represents a property on a class type, QQmlProperty encapsulates
a property on a specific object instance.  To read a property's value, programmers create a
QQmlProperty instance and call the read() method.  Likewise to write a property value the
write() method is used.

For example, for the following QML code:

\qml
// MyItem.qml
import QtQuick 2.0

Text { text: "A bit of text" }
\endqml

The \l Text object's properties could be accessed using QQmlProperty, like this:

\code
#include <QQmlProperty>
#include <QGraphicsObject>

...

QQuickView view(QUrl::fromLocalFile("MyItem.qml"));
QQmlProperty property(view.rootObject(), "font.pixelSize");
qWarning() << "Current pixel size:" << property.read().toInt();
property.write(24);
qWarning() << "Pixel size should now be 24:" << property.read().toInt();
\endcode
*/

/*!
    Create an invalid QQmlProperty.
*/
QQmlProperty::QQmlProperty() = default;

/*!  \internal */
QQmlProperty::~QQmlProperty()
{
    if (d)
        d->release();
    d = nullptr;
}

/*!
    Creates a QQmlProperty for the default property of \a obj. If there is no
    default property, an invalid QQmlProperty will be created.
 */
QQmlProperty::QQmlProperty(QObject *obj)
: d(new QQmlPropertyPrivate)
{
    d->initDefault(obj);
}

/*!
    Creates a QQmlProperty for the default property of \a obj
    using the \l{QQmlContext} {context} \a ctxt. If there is
    no default property, an invalid QQmlProperty will be
    created.
 */
QQmlProperty::QQmlProperty(QObject *obj, QQmlContext *ctxt)
: d(new QQmlPropertyPrivate)
{
    if (ctxt) {
        d->context = QQmlContextData::get(ctxt);
        d->engine = ctxt->engine();
    }
    d->initDefault(obj);
}

/*!
    Creates a QQmlProperty for the default property of \a obj
    using the environment for instantiating QML components that is
    provided by \a engine.  If there is no default property, an
    invalid QQmlProperty will be created.
 */
QQmlProperty::QQmlProperty(QObject *obj, QQmlEngine *engine)
  : d(new QQmlPropertyPrivate)
{
    d->engine = engine;
    d->initDefault(obj);
}

/*!
    Initialize from the default property of \a obj
*/
void QQmlPropertyPrivate::initDefault(QObject *obj)
{
    if (!obj)
        return;

    QMetaProperty p = QQmlMetaType::defaultProperty(obj);
    core.load(p);
    if (core.isValid())
        object = obj;
}

/*!
    Creates a QQmlProperty for the property \a name of \a obj.
 */
QQmlProperty::QQmlProperty(QObject *obj, const QString &name)
: d(new QQmlPropertyPrivate)
{
    d->initProperty(obj, name);
    if (!isValid()) d->object = nullptr;
}

/*!
    Creates a QQmlProperty for the property \a name of \a obj
    using the \l{QQmlContext} {context} \a ctxt.

    Creating a QQmlProperty without a context will render some
    properties - like attached properties - inaccessible.
*/
QQmlProperty::QQmlProperty(QObject *obj, const QString &name, QQmlContext *ctxt)
: d(new QQmlPropertyPrivate)
{
    if (ctxt) {
        d->context = QQmlContextData::get(ctxt);
        d->engine = ctxt->engine();
    }

    d->initProperty(obj, name);
    if (!isValid()) {
        d->object = nullptr;
        d->context.reset();
        d->engine = nullptr;
    }
}

/*!
    Creates a QQmlProperty for the property \a name of \a obj
    using the environment for instantiating QML components that is
    provided by \a engine.
 */
QQmlProperty::QQmlProperty(QObject *obj, const QString &name, QQmlEngine *engine)
: d(new QQmlPropertyPrivate)
{
    d->engine = engine;
    d->initProperty(obj, name);
    if (!isValid()) {
        d->object = nullptr;
        d->context.reset();
        d->engine = nullptr;
    }
}

QQmlProperty QQmlPropertyPrivate::create(QObject *target, const QString &propertyName,
                                         const QQmlRefPointer<QQmlContextData> &context,
                                         QQmlPropertyPrivate::InitFlags flags)
{
    QQmlProperty result;
    auto d = new QQmlPropertyPrivate;
    result.d = d;
    d->context = context;
    d->engine = context ? context->engine() : nullptr;
    d->initProperty(target, propertyName, flags);
    if (!result.isValid()) {
        d->object = nullptr;
        d->context.reset();
        d->engine = nullptr;
    }
    return result;
}

bool QQmlPropertyPrivate::resolveUrlsOnAssignment()
{
    return ::compatResolveUrlsOnAssigment();
}

QQmlRefPointer<QQmlContextData> QQmlPropertyPrivate::effectiveContext() const
{
    if (context)
        return context;
    else if (engine)
        return QQmlContextData::get(engine->rootContext());
    else
        return nullptr;
}

// ### Qt7: Do not accept the "onFoo" syntax for signals anymore, and change the flags accordingly.
void QQmlPropertyPrivate::initProperty(QObject *obj, const QString &name,
                                       QQmlPropertyPrivate::InitFlags flags)
{
    QQmlRefPointer<QQmlTypeNameCache> typeNameCache = context ? context->imports() : nullptr;

    QObject *currentObject = obj;
    QList<QStringView> path;
    QStringView terminal(name);

    if (name.contains(QLatin1Char('.'))) {
        path = QStringView{name}.split(QLatin1Char('.'));
        if (path.isEmpty()) return;

        // Everything up to the last property must be an "object type" property
        for (int ii = 0; ii < path.size() - 1; ++ii) {
            const QStringView &pathName = path.at(ii);

            // Types must begin with an uppercase letter (see checkRegistration()
            // in qqmlmetatype.cpp for the enforcement of this).
            if (typeNameCache && !pathName.isEmpty() && pathName.at(0).isUpper()) {
                QQmlTypeNameCache::Result r = typeNameCache->query(pathName);
                if (r.isValid()) {
                    if (r.type.isValid()) {
                        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
                        QQmlAttachedPropertiesFunc func = r.type.attachedPropertiesFunction(enginePrivate);
                        if (!func) return; // Not an attachable type

                        currentObject = qmlAttachedPropertiesObject(currentObject, func);
                        if (!currentObject) return; // Something is broken with the attachable type
                    } else if (r.importNamespace) {
                        if (++ii == path.size())
                            return; // No type following the namespace

                        // TODO: Do we really _not_ want to query the namespaced types here?
                        r = typeNameCache->query<QQmlTypeNameCache::QueryNamespaced::No>(
                                    path.at(ii), r.importNamespace);

                        if (!r.type.isValid())
                            return; // Invalid type in namespace

                        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
                        QQmlAttachedPropertiesFunc func = r.type.attachedPropertiesFunction(enginePrivate);
                        if (!func)
                            return; // Not an attachable type

                        currentObject = qmlAttachedPropertiesObject(currentObject, func);
                        if (!currentObject)
                            return; // Something is broken with the attachable type

                    } else if (r.scriptIndex != -1) {
                        return; // Not a type
                    } else {
                        Q_ASSERT(!"Unreachable");
                    }
                    continue;
                }

            }

            QQmlPropertyData local;
            const QQmlPropertyData *property = currentObject
                    ? QQmlPropertyCache::property(currentObject, pathName, context, &local)
                    : nullptr;

            if (!property) {
                // Not a property; Might be an ID
                // You can't look up an ID on a non-null object, though.
                if (currentObject || !(flags & InitFlag::AllowId))
                    return;

                for (auto idContext = context; idContext; idContext = idContext->parent()) {
                    const int objectId = idContext->propertyIndex(pathName.toString());
                    if (objectId != -1 && objectId < idContext->numIdValues()) {
                        currentObject = context->idValue(objectId);
                        break;
                    }
                }

                if (!currentObject)
                    return;

                continue;
            } else if (property->isFunction()) {
                return; // Not an object property
            }

            if (ii == (path.size() - 2) && QQmlMetaType::isValueType(property->propType())) {
                // We're now at a value type property
                const QMetaObject *valueTypeMetaObject = QQmlMetaType::metaObjectForValueType(property->propType());
                if (!valueTypeMetaObject) return; // Not a value type

                int idx = valueTypeMetaObject->indexOfProperty(path.last().toUtf8().constData());
                if (idx == -1) return; // Value type property does not exist

                QMetaProperty vtProp = valueTypeMetaObject->property(idx);

                Q_ASSERT(idx <= 0x0000FFFF);

                object = currentObject;
                core = *property;
                valueTypeData.setFlags(QQmlPropertyData::flagsForProperty(vtProp));
                valueTypeData.setPropType(vtProp.metaType());
                valueTypeData.setCoreIndex(idx);

                return;
            } else {
                if (!property->isQObject()) {
                    if (auto asPropertyMap = qobject_cast<QQmlPropertyMap*>(currentObject))
                        currentObject = asPropertyMap->value(path.at(ii).toString()).value<QObject*>();
                    else
                        return; // Not an object property, and not a property map
                } else {
                    property->readProperty(currentObject, &currentObject);
                }

                if (!currentObject) return; // No value

            }

        }

        terminal = path.last();
    } else if (!currentObject) {
        return;
    }

    auto findSignalInMetaObject = [&](const QByteArray &signalName) {
        const QMetaMethod method = findSignalByName(currentObject->metaObject(), signalName);
        if (!method.isValid())
            return false;

        object = currentObject;
        core.load(method);
        return true;
    };

    QQmlData *ddata = QQmlData::get(currentObject, false);
    auto findChangeSignal = [&](QStringView signalName) {
        const QString changed = QStringLiteral("Changed");
        if (signalName.endsWith(changed)) {
            const QStringView propName = signalName.first(signalName.size() - changed.size());
            const QQmlPropertyData *d = ddata->propertyCache->property(propName, currentObject, context);
            while (d && d->isFunction())
                d = ddata->propertyCache->overrideData(d);

            if (d && d->notifyIndex() != -1) {
                object = currentObject;
                core = *ddata->propertyCache->signal(d->notifyIndex());
                return true;
            }
        }
        return false;
    };

    static constexpr QLatin1String On("on");
    static constexpr qsizetype StrlenOn = On.size();

    const QString terminalString = terminal.toString();
    if (QmlIR::IRBuilder::isSignalPropertyName(terminalString)) {
        QString signalName = terminalString.mid(2);
        int firstNon_;
        int length = signalName.size();
        for (firstNon_ = 0; firstNon_ < length; ++firstNon_)
            if (signalName.at(firstNon_) != u'_')
                break;
        signalName[firstNon_] = signalName.at(firstNon_).toLower();

        if (ddata && ddata->propertyCache) {
            // Try method
            const QQmlPropertyData *d = ddata->propertyCache->property(
                        signalName, currentObject, context);

            // ### Qt7: This code treats methods as signals. It should use d->isSignal().
            //          That would be a change in behavior, though. Right now you can construct a
            //          QQmlProperty from such a thing.
            while (d && !d->isFunction())
                d = ddata->propertyCache->overrideData(d);

            if (d) {
                object = currentObject;
                core = *d;
                return;
            }

            if (findChangeSignal(signalName))
                return;
        } else if (findSignalInMetaObject(signalName.toUtf8())) {
            return;
        }
    } else if (terminalString.size() > StrlenOn && terminalString.startsWith(On)) {
        // This is quite wrong. But we need it for backwards compatibility.
        QString signalName = terminalString.sliced(2);
        signalName.front() = signalName.front().toLower();

        QString handlerName = On + signalName;
        const auto end = handlerName.end();
        auto result = std::find_if(
                std::next(handlerName.begin(), StrlenOn), end,
                [](const QChar &c) { return c.isLetter(); });
        if (result != end)
            *result = result->toUpper();

        qWarning()
                << terminalString
                << "is not a properly capitalized signal handler name."
                << handlerName
                << "would be correct.";
        if (findSignalInMetaObject(signalName.toUtf8()))
            return;
    }

    if (ddata && ddata->propertyCache) {
        const QQmlPropertyData *property = ddata->propertyCache->property(
                    terminal, currentObject, context);

        // Technically, we might find an override that is not a function.
        while (property && !property->isSignal()) {
            if (!property->isFunction()) {
                object = currentObject;
                core = *property;
                nameCache = terminalString;
                return;
            }
            property = ddata->propertyCache->overrideData(property);
        }

        if (!(flags & InitFlag::AllowSignal))
            return;

        if (property) {
            Q_ASSERT(property->isSignal());
            object = currentObject;
            core = *property;
            return;
        }

        // At last: Try the change signal.
        findChangeSignal(terminal);
    } else {
        // We might still find the property in the metaobject, even without property cache.
        const QByteArray propertyName = terminal.toUtf8();
        const QMetaProperty prop = findPropertyByName(currentObject->metaObject(), propertyName);

        if (prop.isValid()) {
            object = currentObject;
            core.load(prop);
            return;
        }

        if (flags & InitFlag::AllowSignal)
            findSignalInMetaObject(terminal.toUtf8());
    }
}

/*! \internal
    Returns the index of this property's signal, in the signal index range
    (see QObjectPrivate::signalIndex()). This is different from
    QMetaMethod::methodIndex().
*/
int QQmlPropertyPrivate::signalIndex() const
{
    Q_ASSERT(type() == QQmlProperty::SignalProperty);
    QMetaMethod m = object->metaObject()->method(core.coreIndex());
    return QMetaObjectPrivate::signalIndex(m);
}

/*!
    Create a copy of \a other.
*/
QQmlProperty::QQmlProperty(const QQmlProperty &other)
{
    d = other.d;
    if (d)
        d->addref();
}

/*!
  \enum QQmlProperty::PropertyTypeCategory

  This enum specifies a category of QML property.

  \value InvalidCategory The property is invalid, or is a signal property.
  \value List The property is a QQmlListProperty list property
  \value Object The property is a QObject derived type pointer
  \value Normal The property is a normal value property.
 */

/*!
  \enum QQmlProperty::Type

  This enum specifies a type of QML property.

  \value Invalid The property is invalid.
  \value Property The property is a regular Qt property.
  \value SignalProperty The property is a signal property.
*/

/*!
    Returns the property category.
*/
QQmlProperty::PropertyTypeCategory QQmlProperty::propertyTypeCategory() const
{
    return d ? d->propertyTypeCategory() : InvalidCategory;
}

QQmlProperty::PropertyTypeCategory
QQmlPropertyPrivate::propertyTypeCategory() const
{
    uint type = this->type();

    if (isValueType()) {
        return QQmlProperty::Normal;
    } else if (type & QQmlProperty::Property) {
        QMetaType type = propertyType();
        if (!type.isValid())
            return QQmlProperty::InvalidCategory;
        else if (QQmlMetaType::isValueType(type))
            return QQmlProperty::Normal;
        else if (core.isQObject())
            return QQmlProperty::Object;
        else if (core.isQList())
            return QQmlProperty::List;
        else
            return QQmlProperty::Normal;
    }

    return QQmlProperty::InvalidCategory;
}

/*!
    Returns the type name of the property, or 0 if the property has no type
    name.
*/
const char *QQmlProperty::propertyTypeName() const
{
    if (!d)
        return nullptr;
    if (d->isValueType()) {
        const QMetaObject *valueTypeMetaObject = QQmlMetaType::metaObjectForValueType(d->core.propType());
        Q_ASSERT(valueTypeMetaObject);
        return valueTypeMetaObject->property(d->valueTypeData.coreIndex()).typeName();
    } else if (d->object && type() & Property && d->core.isValid()) {
        return d->object->metaObject()->property(d->core.coreIndex()).typeName();
    } else {
        return nullptr;
    }
}

/*!
    Returns true if \a other and this QQmlProperty represent the same
    property.
*/
bool QQmlProperty::operator==(const QQmlProperty &other) const
{
    if (!d || !other.d)
        return false;
    // category is intentially omitted here as it is generated
    // from the other members
    return d->object == other.d->object &&
           d->core.coreIndex() == other.d->core.coreIndex() &&
           d->valueTypeData.coreIndex() == other.d->valueTypeData.coreIndex();
}

/*!
    Returns the metatype id of the property, or QMetaType::UnknownType if the
    property has no metatype.

    \sa propertyMetaType
*/
int QQmlProperty::propertyType() const
{
    return d ? d->propertyType().id() : int(QMetaType::UnknownType);
}

/*!
     Returns the metatype of the property.

     \sa propertyType
 */
QMetaType QQmlProperty::propertyMetaType() const
{
    return d ? d->propertyType() : QMetaType {};
}

bool QQmlPropertyPrivate::isValueType() const
{
    return valueTypeData.isValid();
}

QMetaType QQmlPropertyPrivate::propertyType() const
{
    uint type = this->type();
    if (isValueType()) {
        return valueTypeData.propType();
    } else if (type & QQmlProperty::Property) {
        return core.propType();
    } else {
        return QMetaType();
    }
}

QQmlProperty::Type QQmlPropertyPrivate::type() const
{
    if (core.isFunction())
        return QQmlProperty::SignalProperty;
    else if (core.isValid())
        return QQmlProperty::Property;
    else
        return QQmlProperty::Invalid;
}

/*!
    Returns the type of the property.
*/
QQmlProperty::Type QQmlProperty::type() const
{
    return d ? d->type() : Invalid;
}

/*!
    Returns true if this QQmlProperty represents a regular Qt property.
*/
bool QQmlProperty::isProperty() const
{
    return type() & Property;
}

/*!
    Returns true if this QQmlProperty represents a QML signal property.
*/
bool QQmlProperty::isSignalProperty() const
{
    return type() & SignalProperty;
}

/*!
    Returns the QQmlProperty's QObject.
*/
QObject *QQmlProperty::object() const
{
    return d ? d->object : nullptr;
}

/*!
    Assign \a other to this QQmlProperty.
*/
QQmlProperty &QQmlProperty::operator=(const QQmlProperty &other)
{
    QQmlProperty copied(other);
    qSwap(d, copied.d);
    return *this;
}

/*!
    Returns true if the property is writable, otherwise false.
*/
bool QQmlProperty::isWritable() const
{
    if (!d)
        return false;
    if (!d->object)
        return false;
    if (d->core.isQList())           //list
        return true;
    else if (d->core.isFunction())   //signal handler
        return false;
    else if (d->core.isValid())      //normal property
        return d->core.isWritable();
    else
        return false;
}

/*!
    \internal
    Returns true if the property is bindable, otherwise false.
 */
bool QQmlProperty::isBindable() const
{
    if (!d)
        return false;
    if (!d->object)
        return false;
    if (d->core.isValid())
        return d->core.isBindable();
    return false;
}

/*!
    Returns true if the property is designable, otherwise false.
*/
bool QQmlProperty::isDesignable() const
{
    if (!d)
        return false;
    if (type() & Property && d->core.isValid() && d->object)
        return d->object->metaObject()->property(d->core.coreIndex()).isDesignable();
    else
        return false;
}

/*!
    Returns true if the property is resettable, otherwise false.
*/
bool QQmlProperty::isResettable() const
{
    if (!d)
        return false;
    if (type() & Property && d->core.isValid() && d->object)
        return d->core.isResettable();
    else
        return false;
}

/*!
    Returns true if the QQmlProperty refers to a valid property, otherwise
    false.
*/
bool QQmlProperty::isValid() const
{
    if (!d)
        return false;
    return type() != Invalid;
}

/*!
    Return the name of this QML property.
*/
QString QQmlProperty::name() const
{
    if (!d)
        return QString();
    if (d->nameCache.isEmpty()) {
        if (!d->object) {
        } else if (d->isValueType()) {
            const QMetaObject *valueTypeMetaObject = QQmlMetaType::metaObjectForValueType(d->core.propType());
            Q_ASSERT(valueTypeMetaObject);

            const char *vtName = valueTypeMetaObject->property(d->valueTypeData.coreIndex()).name();
            d->nameCache = d->core.name(d->object) + QLatin1Char('.') + QString::fromUtf8(vtName);
        } else if (type() & SignalProperty) {
            // ### Qt7: Return the original signal name here. Do not prepend "on"
            QString name = QStringLiteral("on") + d->core.name(d->object);
            for (int i = 2, end = name.size(); i != end; ++i) {
                const QChar c = name.at(i);
                if (c != u'_') {
                    name[i] = c.toUpper();
                    break;
                }
            }
            d->nameCache = name;
        } else {
            d->nameCache = d->core.name(d->object);
        }
    }

    return d->nameCache;
}

/*!
  Returns the \l{QMetaProperty} {Qt property} associated with
  this QML property.
 */
QMetaProperty QQmlProperty::property() const
{
    if (!d)
        return QMetaProperty();
    if (type() & Property && d->core.isValid() && d->object)
        return d->object->metaObject()->property(d->core.coreIndex());
    else
        return QMetaProperty();
}

/*!
    Return the QMetaMethod for this property if it is a SignalProperty,
    otherwise returns an invalid QMetaMethod.
*/
QMetaMethod QQmlProperty::method() const
{
    if (!d)
        return QMetaMethod();
    if (type() & SignalProperty && d->object)
        return d->object->metaObject()->method(d->core.coreIndex());
    else
        return QMetaMethod();
}

/*!
    Returns the binding associated with this property, or 0 if no binding
    exists.
*/
QQmlAbstractBinding *
QQmlPropertyPrivate::binding(const QQmlProperty &that)
{
    if (!that.d || !that.isProperty() || !that.d->object)
        return nullptr;

    QQmlPropertyIndex thatIndex(that.d->core.coreIndex(), that.d->valueTypeData.coreIndex());
    return binding(that.d->object, thatIndex);
}

/*!
    Set the binding associated with this property to \a newBinding.  Returns
    the existing binding (if any), otherwise 0.

    \a newBinding will be enabled, and the returned binding (if any) will be
    disabled.

    Ownership of \a newBinding transfers to QML.  Ownership of the return value
    is assumed by the caller.
*/
void
QQmlPropertyPrivate::setBinding(const QQmlProperty &that, QQmlAbstractBinding *newBinding)
{
    if (!newBinding) {
        removeBinding(that);
        return;
    }

    if (!that.d || !that.isProperty() || !that.d->object) {
        if (!newBinding->ref)
            delete newBinding;
        return;
    }
    setBinding(newBinding);
}

static void removeOldBinding(QObject *object, QQmlPropertyIndex index, QQmlPropertyPrivate::BindingFlags flags = QQmlPropertyPrivate::None)
{
    int coreIndex = index.coreIndex();
    int valueTypeIndex = index.valueTypeIndex();

    QQmlData *data = QQmlData::get(object, false);

    if (!data || !data->hasBindingBit(coreIndex))
        return;

    QQmlAbstractBinding::Ptr oldBinding;
    oldBinding = data->bindings;

    while (oldBinding && (oldBinding->targetPropertyIndex().coreIndex() != coreIndex ||
                          oldBinding->targetPropertyIndex().hasValueTypeIndex()))
        oldBinding = oldBinding->nextBinding();

    if (!oldBinding)
        return;

    if (valueTypeIndex != -1 && oldBinding->kind() == QQmlAbstractBinding::ValueTypeProxy)
        oldBinding = static_cast<QQmlValueTypeProxyBinding *>(oldBinding.data())->binding(index);

    if (!oldBinding)
        return;

    if (!(flags & QQmlPropertyPrivate::DontEnable))
        oldBinding->setEnabled(false, {});
    oldBinding->removeFromObject();
}

void QQmlPropertyPrivate::removeBinding(QQmlAbstractBinding *b)
{
    removeBinding(b->targetObject(), b->targetPropertyIndex());
}

void QQmlPropertyPrivate::removeBinding(QObject *o, QQmlPropertyIndex index)
{
    Q_ASSERT(o);

    auto [target, targetIndex] = findAliasTarget(o, index);
    removeOldBinding(target, targetIndex);
}

void QQmlPropertyPrivate::removeBinding(const QQmlProperty &that)
{
    if (!that.d || !that.isProperty() || !that.d->object)
        return;

    removeBinding(that.d->object, that.d->encodedIndex());
}

QQmlAbstractBinding *
QQmlPropertyPrivate::binding(QObject *object, QQmlPropertyIndex index)
{
    auto aliasTarget = findAliasTarget(object, index);
    object = aliasTarget.targetObject;
    index = aliasTarget.targetIndex;

    QQmlData *data = QQmlData::get(object);
    if (!data)
        return nullptr;

    const int coreIndex = index.coreIndex();
    const int valueTypeIndex = index.valueTypeIndex();

    if (coreIndex < 0 || !data->hasBindingBit(coreIndex))
        return nullptr;

    QQmlAbstractBinding *binding = data->bindings;
    while (binding && (binding->targetPropertyIndex().coreIndex() != coreIndex ||
                       binding->targetPropertyIndex().hasValueTypeIndex()))
        binding = binding->nextBinding();

    if (binding && valueTypeIndex != -1) {
        if (binding->kind() == QQmlAbstractBinding::ValueTypeProxy)
            binding = static_cast<QQmlValueTypeProxyBinding *>(binding)->binding(index);
    }

    return binding;
}

void QQmlPropertyPrivate::findAliasTarget(QObject *object, QQmlPropertyIndex bindingIndex,
                                          QObject **targetObject,
                                          QQmlPropertyIndex *targetBindingIndex)
{
    QQmlData *data = QQmlData::get(object, false);
    if (data) {
        int coreIndex = bindingIndex.coreIndex();
        int valueTypeIndex = bindingIndex.valueTypeIndex();

        const QQmlPropertyData *propertyData =
            data->propertyCache?data->propertyCache->property(coreIndex):nullptr;
        if (propertyData && propertyData->isAlias()) {
            QQmlVMEMetaObject *vme = QQmlVMEMetaObject::getForProperty(object, coreIndex);

            QObject *aObject = nullptr; int aCoreIndex = -1; int aValueTypeIndex = -1;
            if (vme->aliasTarget(coreIndex, &aObject, &aCoreIndex, &aValueTypeIndex)) {
                // This will either be a value type sub-reference or an alias to a value-type sub-reference not both
                Q_ASSERT(valueTypeIndex == -1 || aValueTypeIndex == -1);

                QQmlPropertyIndex aBindingIndex(aCoreIndex);
                if (aValueTypeIndex != -1) {
                    aBindingIndex = QQmlPropertyIndex(aCoreIndex, aValueTypeIndex);
                } else if (valueTypeIndex != -1) {
                    aBindingIndex = QQmlPropertyIndex(aCoreIndex, valueTypeIndex);
                }

                findAliasTarget(aObject, aBindingIndex, targetObject, targetBindingIndex);
                return;
            }
        }
    }

    *targetObject = object;
    *targetBindingIndex = bindingIndex;
}

QQmlPropertyPrivate::ResolvedAlias QQmlPropertyPrivate::findAliasTarget(QObject *baseObject, QQmlPropertyIndex baseIndex)
{
    ResolvedAlias resolved;
    findAliasTarget(baseObject, baseIndex, &resolved.targetObject, &resolved.targetIndex);
    return resolved;
}



void QQmlPropertyPrivate::setBinding(QQmlAbstractBinding *binding, BindingFlags flags, QQmlPropertyData::WriteFlags writeFlags)
{
    Q_ASSERT(binding);
    Q_ASSERT(binding->targetObject());

    QObject *object = binding->targetObject();
    const QQmlPropertyIndex index = binding->targetPropertyIndex();

#ifndef QT_NO_DEBUG
    int coreIndex = index.coreIndex();
    QQmlData *data = QQmlData::get(object, true);
    if (data->propertyCache) {
        const QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
        Q_ASSERT(propertyData);
    }
#endif

    removeOldBinding(object, index, flags);

    binding->addToObject();
    if (!(flags & DontEnable))
        binding->setEnabled(true, writeFlags);

}

/*!
    Returns the expression associated with this signal property, or 0 if no
    signal expression exists.
*/
QQmlBoundSignalExpression *
QQmlPropertyPrivate::signalExpression(const QQmlProperty &that)
{
    if (!(that.type() & QQmlProperty::SignalProperty))
        return nullptr;

    if (!that.d->object)
        return nullptr;
    QQmlData *data = QQmlData::get(that.d->object);
    if (!data)
        return nullptr;

    QQmlBoundSignal *signalHandler = data->signalHandlers;

    while (signalHandler && signalHandler->signalIndex() != QQmlPropertyPrivate::get(that)->signalIndex())
        signalHandler = signalHandler->m_nextSignal;

    if (signalHandler)
        return signalHandler->expression();

    return nullptr;
}

/*!
    Set the signal expression associated with this signal property to \a expr.
    A reference to \a expr will be added by QML.
*/
void QQmlPropertyPrivate::setSignalExpression(const QQmlProperty &that, QQmlBoundSignalExpression *expr)
{
    if (expr)
        expr->addref();
    QQmlPropertyPrivate::takeSignalExpression(that, expr);
}

/*!
    Set the signal expression associated with this signal property to \a expr.
    Ownership of \a expr transfers to QML.
*/
void QQmlPropertyPrivate::takeSignalExpression(const QQmlProperty &that,
                                         QQmlBoundSignalExpression *expr)
{
    if (!(that.type() & QQmlProperty::SignalProperty)) {
        if (expr)
            expr->release();
        return;
    }

    if (!that.d->object)
        return;
    QQmlData *data = QQmlData::get(that.d->object, nullptr != expr);
    if (!data)
        return;

    QQmlBoundSignal *signalHandler = data->signalHandlers;

    while (signalHandler && signalHandler->signalIndex() != QQmlPropertyPrivate::get(that)->signalIndex())
        signalHandler = signalHandler->m_nextSignal;

    if (signalHandler) {
        signalHandler->takeExpression(expr);
        return;
    }

    if (expr) {
        int signalIndex = QQmlPropertyPrivate::get(that)->signalIndex();
        QQmlBoundSignal *signal = new QQmlBoundSignal(that.d->object, signalIndex, that.d->object,
                                                      expr->engine());
        signal->takeExpression(expr);
    }
}

/*!
    Returns the property value.
*/
QVariant QQmlProperty::read() const
{
    if (!d)
        return QVariant();
    if (!d->object)
        return QVariant();

    if (type() & SignalProperty) {

        return QVariant();

    } else if (type() & Property) {

        return d->readValueProperty();

    }
    return QVariant();
}

/*!
Return the \a name property value of \a object.  This method is equivalent to:
\code
    QQmlProperty p(object, name);
    p.read();
\endcode
*/
QVariant QQmlProperty::read(const QObject *object, const QString &name)
{
    QQmlProperty p(const_cast<QObject *>(object), name);
    return p.read();
}

/*!
  Return the \a name property value of \a object using the
  \l{QQmlContext} {context} \a ctxt.  This method is
  equivalent to:

  \code
    QQmlProperty p(object, name, context);
    p.read();
  \endcode
*/
QVariant QQmlProperty::read(const QObject *object, const QString &name, QQmlContext *ctxt)
{
    QQmlProperty p(const_cast<QObject *>(object), name, ctxt);
    return p.read();
}

/*!

  Return the \a name property value of \a object using the environment
  for instantiating QML components that is provided by \a engine. .
  This method is equivalent to:

  \code
    QQmlProperty p(object, name, engine);
    p.read();
  \endcode
*/
QVariant QQmlProperty::read(const QObject *object, const QString &name, QQmlEngine *engine)
{
    QQmlProperty p(const_cast<QObject *>(object), name, engine);
    return p.read();
}

QVariant QQmlPropertyPrivate::readValueProperty()
{
    auto doRead = [&](QQmlGadgetPtrWrapper *wrapper) {
        wrapper->read(object, core.coreIndex());
        return wrapper->readOnGadget(wrapper->property(valueTypeData.coreIndex()));
    };

    if (isValueType()) {
        if (QQmlGadgetPtrWrapper *wrapper = QQmlGadgetPtrWrapper::instance(engine, core.propType()))
            return doRead(wrapper);
        if (QQmlValueType *valueType = QQmlMetaType::valueType(core.propType())) {
            QQmlGadgetPtrWrapper wrapper(valueType, nullptr);
            return doRead(&wrapper);
        }
        return QVariant();
    } else if (core.isQList()) {

        QQmlListProperty<QObject> prop;
        core.readProperty(object, &prop);
        return QVariant::fromValue(QQmlListReferencePrivate::init(prop, core.propType()));

    } else if (core.isQObject()) {

        QObject *rv = nullptr;
        core.readProperty(object, &rv);
        return QVariant::fromValue(rv);

    } else {

        if (!core.propType().isValid()) // Unregistered type
            return object->metaObject()->property(core.coreIndex()).read(object);

        QVariant value;
        int status = -1;
        void *args[] = { nullptr, &value, &status };
        if (core.propType() == QMetaType::fromType<QVariant>()) {
            args[0] = &value;
        } else {
            value = QVariant(core.propType(), (void*)nullptr);
            args[0] = value.data();
        }
        core.readPropertyWithArgs(object, args);
        if (core.propType() != QMetaType::fromType<QVariant>() && args[0] != value.data())
            return QVariant(QMetaType(core.propType()), args[0]);

        return value;
    }
}

// helper function to allow assignment / binding to QList<QUrl> properties.
QList<QUrl> QQmlPropertyPrivate::urlSequence(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QList<QUrl>>())
        return value.value<QList<QUrl> >();

    QList<QUrl> urls;
    if (value.metaType() == QMetaType::fromType<QUrl>()) {
        urls.append(value.toUrl());
    } else if (value.metaType() == QMetaType::fromType<QString>()) {
        urls.append(QUrl(value.toString()));
    } else if (value.metaType() == QMetaType::fromType<QByteArray>()) {
        urls.append(QUrl(QString::fromUtf8(value.toByteArray())));
    } else if (value.metaType() == QMetaType::fromType<QStringList>()) {
        QStringList urlStrings = value.value<QStringList>();
        const int urlStringsSize = urlStrings.size();
        urls.reserve(urlStringsSize);
        for (int i = 0; i < urlStringsSize; ++i)
            urls.append(QUrl(urlStrings.at(i)));
    } // note: QList<QByteArray> is not currently supported.
    return urls;
}

// ### Qt7: Get rid of this
QList<QUrl> QQmlPropertyPrivate::urlSequence(
            const QVariant &value, const QQmlRefPointer<QQmlContextData> &ctxt)
{
    QList<QUrl> urls = urlSequence(value);

    for (auto urlIt = urls.begin(); urlIt != urls.end(); ++urlIt)
        *urlIt = ctxt->resolvedUrl(*urlIt);

    return urls;
}

//writeEnumProperty MIRRORS the relelvant bit of QMetaProperty::write AND MUST BE KEPT IN SYNC!
bool QQmlPropertyPrivate::writeEnumProperty(const QMetaProperty &prop, int idx, QObject *object, const QVariant &value, int flags)
{
    if (!object || !prop.isWritable())
        return false;

    QVariant v = value;
    if (prop.isEnumType() && v.metaType() != prop.metaType()) {
        QMetaEnum menum = prop.enumerator();
        if (v.userType() == QMetaType::QString) {
            bool ok;
            if (prop.isFlagType())
                v = QVariant(menum.keysToValue(value.toByteArray(), &ok));
            else
                v = QVariant(menum.keyToValue(value.toByteArray(), &ok));
            if (!ok)
                return false;
        }
        if (!v.convert(prop.metaType())) // ### TODO: underlyingType might be faster?
            return false;
    }

    // the status variable is changed by qt_metacall to indicate what it did
    // this feature is currently only used by QtDBus and should not be depended
    // upon. Don't change it without looking into QDBusAbstractInterface first
    // -1 (unchanged): normal qt_metacall, result stored in argv[0]
    // changed: result stored directly in value, return the value of status
    int status = -1;
    void *argv[] = { v.data(), &v, &status, &flags };
    QMetaObject::metacall(object, QMetaObject::WriteProperty, idx, argv);
    return status;
}

bool QQmlPropertyPrivate::writeValueProperty(const QVariant &value, QQmlPropertyData::WriteFlags flags)
{
    return writeValueProperty(object, core, valueTypeData, value, effectiveContext(), flags);
}

static void removeValuePropertyBinding(
            QObject *object, const QQmlPropertyData &core,
            const QQmlPropertyData &valueTypeData, QQmlPropertyData::WriteFlags flags)
{
    // Remove any existing bindings on this property
    if (!(flags & QQmlPropertyData::DontRemoveBinding) && object) {
        QQmlPropertyPrivate::removeBinding(
                    object, QQmlPropertyPrivate::encodedIndex(core, valueTypeData));
    }
}

template<typename Op>
bool changePropertyAndWriteBack(
            QObject *object, int coreIndex, QQmlGadgetPtrWrapper *wrapper,
            QQmlPropertyData::WriteFlags flags, int internalIndex, Op op)
{
    wrapper->read(object, coreIndex);
    const bool rv = op(wrapper);
    wrapper->write(object, coreIndex, flags, internalIndex);
    return rv;
}

template<typename Op>
bool changeThroughGadgetPtrWrapper(
        QObject *object, const QQmlPropertyData &core,
        const QQmlRefPointer<QQmlContextData> &context, QQmlPropertyData::WriteFlags flags,
        int internalIndex, Op op)
{
    if (QQmlGadgetPtrWrapper *wrapper = context
            ? QQmlGadgetPtrWrapper::instance(context->engine(), core.propType())
            : nullptr) {
        return changePropertyAndWriteBack(
                    object, core.coreIndex(), wrapper, flags, internalIndex, op);
    }

    if (QQmlValueType *valueType = QQmlMetaType::valueType(core.propType())) {
        QQmlGadgetPtrWrapper wrapper(valueType, nullptr);
        return changePropertyAndWriteBack(
                    object, core.coreIndex(), &wrapper, flags, internalIndex, op);
    }

    return false;
}

bool QQmlPropertyPrivate::writeValueProperty(
            QObject *object, const QQmlPropertyData &core, const QQmlPropertyData &valueTypeData,
            const QVariant &value, const QQmlRefPointer<QQmlContextData> &context,
            QQmlPropertyData::WriteFlags flags)
{
    removeValuePropertyBinding(object, core, valueTypeData, flags);

    if (!valueTypeData.isValid())
        return write(object, core, value, context, flags);

    return changeThroughGadgetPtrWrapper(
                object, core, context, flags | QQmlPropertyData::HasInternalIndex,
                valueTypeData.coreIndex(), [&](QQmlGadgetPtrWrapper *wrapper) {
        return write(wrapper, valueTypeData, value, context, flags);
    });
}

bool QQmlPropertyPrivate::resetValueProperty(
            QObject *object, const QQmlPropertyData &core, const QQmlPropertyData &valueTypeData,
            const QQmlRefPointer<QQmlContextData> &context, QQmlPropertyData::WriteFlags flags)
{
    removeValuePropertyBinding(object, core, valueTypeData, flags);

    if (!valueTypeData.isValid())
        return reset(object, core, flags);

    return changeThroughGadgetPtrWrapper(
                object, core, context, flags | QQmlPropertyData::HasInternalIndex,
                valueTypeData.coreIndex(), [&](QQmlGadgetPtrWrapper *wrapper) {
        return reset(wrapper, valueTypeData, flags);
    });
}

// We need to prevent new-style bindings from being removed.
struct BindingFixer
{
    Q_DISABLE_COPY_MOVE(BindingFixer);

    BindingFixer(QObject *object, const QQmlPropertyData &property,
                 QQmlPropertyData::WriteFlags flags)
    {
        if (!property.isBindable() || !(flags & QQmlPropertyData::DontRemoveBinding))
            return;

        QUntypedBindable bindable;
        void *argv[] = {&bindable};
        QMetaObject::metacall(object, QMetaObject::BindableProperty, property.coreIndex(), argv);
        untypedBinding = bindable.binding();
        if (auto priv = QPropertyBindingPrivate::get(untypedBinding))
            priv->setSticky(true);
    }

    ~BindingFixer()
    {
        if (untypedBinding.isNull())
            return;
        auto priv = QPropertyBindingPrivate::get(untypedBinding);
        priv->setSticky(false);
    }

private:
    QUntypedPropertyBinding untypedBinding;
};

struct ConvertAndAssignResult {
    bool couldConvert = false;
    bool couldWrite = false;

    operator bool() const { return couldConvert; }
};

static ConvertAndAssignResult tryConvertAndAssign(
        QObject *object, const QQmlPropertyData &property, const QVariant &value,
        QQmlPropertyData::WriteFlags flags, QMetaType propertyMetaType, QMetaType variantMetaType,
        bool isUrl) {

    if (isUrl
            || variantMetaType == QMetaType::fromType<QString>()
            || propertyMetaType == QMetaType::fromType<QList<QUrl>>()
            || property.isQList()) {
        return {false, false};
    }

    if (variantMetaType == QMetaType::fromType<QJSValue>()) {
        // Handle Qt.binding bindings here to avoid mistaken conversion below
        const QJSValue &jsValue = get<QJSValue>(value);
        const QV4::FunctionObject *f
                = QJSValuePrivate::asManagedType<QV4::FunctionObject>(&jsValue);
        if (f && f->isBinding()) {
            QV4::QObjectWrapper::setProperty(
                    f->engine(), object, &property, f->asReturnedValue());
            return {true, true};
        }
    }

    // common cases:
    switch (propertyMetaType.id()) {
    case QMetaType::Bool:
        if (value.canConvert(propertyMetaType)) {
            bool b = value.toBool();
            return {true, property.writeProperty(object, &b, flags)};
        }
        return {false, false};
    case QMetaType::Int: {
        bool ok = false;
        int i = value.toInt(&ok);
        return {ok, ok && property.writeProperty(object, &i, flags)};
    }
    case QMetaType::UInt: {
        bool ok = false;
        uint u = value.toUInt(&ok);
        return {ok, ok && property.writeProperty(object, &u, flags)};
    }
    case QMetaType::Double: {
        bool ok = false;
        double d = value.toDouble(&ok);
        return {ok, ok && property.writeProperty(object, &d, flags)};
    }
    case QMetaType::Float: {
        bool ok = false;
        float f = value.toFloat(&ok);
        return {ok, ok && property.writeProperty(object, &f, flags)};
    }
    case QMetaType::QString:
        if (value.canConvert(propertyMetaType)) {
            QString s = value.toString();
            return {true, property.writeProperty(object, &s, flags)};
        }
        return {false, false};
    case QMetaType::QVariantMap:
        if (value.canConvert(propertyMetaType)) {
            QVariantMap m = value.toMap();
            return {true, property.writeProperty(object, &m, flags)};
        }
        return {false, false};
    default: {
        break;
    }
    }

    QVariant converted = QQmlValueTypeProvider::createValueType(value, propertyMetaType);
    if (!converted.isValid()) {
        converted = QVariant(propertyMetaType);
        if (!QMetaType::convert(value.metaType(), value.constData(),
                                propertyMetaType, converted.data()))  {
            return {false, false};
        }
    }
    return {true, property.writeProperty(object, converted.data(), flags)};
};

template<typename Op>
bool iterateQObjectContainer(QMetaType metaType, const void *data, Op op)
{
    QSequentialIterable iterable;
    if (!QMetaType::convert(metaType, data, QMetaType::fromType<QSequentialIterable>(), &iterable))
        return false;

    const QMetaSequence metaSequence = iterable.metaContainer();

    if (!metaSequence.hasConstIterator()
            || !metaSequence.canGetValueAtConstIterator()
            || !iterable.valueMetaType().flags().testFlag(QMetaType::PointerToQObject)) {
        return false;
    }

    const void *container = iterable.constIterable();
    void *it = metaSequence.constBegin(container);
    const void *end = metaSequence.constEnd(container);
    QObject *o = nullptr;
    while (!metaSequence.compareConstIterator(it, end)) {
        metaSequence.valueAtConstIterator(it, &o);
        op(o);
        metaSequence.advanceConstIterator(it, 1);
    }
    metaSequence.destroyConstIterator(it);
    metaSequence.destroyConstIterator(end);
    return true;
}

bool QQmlPropertyPrivate::write(
        QObject *object, const QQmlPropertyData &property, const QVariant &value,
        const QQmlRefPointer<QQmlContextData> &context, QQmlPropertyData::WriteFlags flags)
{
    const QMetaType propertyMetaType = property.propType();
    const QMetaType variantMetaType = value.metaType();

    const BindingFixer bindingFixer(object, property, flags);

    if (property.isEnum()) {
        QMetaProperty prop = object->metaObject()->property(property.coreIndex());
        QVariant v = value;
        // Enum values come through the script engine as doubles
        if (variantMetaType == QMetaType::fromType<double>()) {
            double integral;
            double fractional = std::modf(value.toDouble(), &integral);
            if (qFuzzyIsNull(fractional))
                v.convert(QMetaType::fromType<qint32>());
        }
        return writeEnumProperty(prop, property.coreIndex(), object, v, flags);
    }

    QQmlEnginePrivate *enginePriv = QQmlEnginePrivate::get(context);
    const bool isUrl = propertyMetaType == QMetaType::fromType<QUrl>(); // handled separately

    // The cases below are in approximate order of likelyhood:
    if (propertyMetaType == variantMetaType && !isUrl
            && propertyMetaType != QMetaType::fromType<QList<QUrl>>() && !property.isQList()) {
        return property.writeProperty(object, const_cast<void *>(value.constData()), flags);
    } else if (property.isQObject()) {
        QVariant val = value;
        QMetaType varType;
        if (variantMetaType == QMetaType::fromType<std::nullptr_t>()) {
            // This reflects the fact that you can assign a nullptr to a QObject pointer
            // Without the change to QObjectStar, rawMetaObjectForType would not give us a QQmlMetaObject
            varType = QMetaType::fromType<QObject*>();
            val = QVariant(varType, nullptr);
        } else {
            varType = variantMetaType;
        }
        QQmlMetaObject valMo = rawMetaObjectForType(varType);
        if (valMo.isNull() || !varType.flags().testFlag(QMetaType::PointerToQObject))
            return false;
        QObject *o = *static_cast<QObject *const *>(val.constData());
        QQmlMetaObject propMo = rawMetaObjectForType(propertyMetaType);

        if (o)
            valMo = o;

        if (QQmlMetaObject::canConvert(valMo, propMo)) {
            return property.writeProperty(object, &o, flags);
        } else if (!o && QQmlMetaObject::canConvert(propMo, valMo)) {
            // In the case of a null QObject, we assign the null if there is
            // any change that the null variant type could be up or down cast to
            // the property type.
            return property.writeProperty(object, &o, flags);
        } else {
            return false;
        }
    } else if (ConvertAndAssignResult result = tryConvertAndAssign(
                   object, property, value, flags, propertyMetaType, variantMetaType, isUrl)) {
        return result.couldWrite;
    } else if (propertyMetaType == QMetaType::fromType<QVariant>()) {
        return property.writeProperty(object, const_cast<QVariant *>(&value), flags);
    } else if (isUrl) {
        QUrl u;
        if (variantMetaType == QMetaType::fromType<QUrl>()) {
            u = value.toUrl();
            if (compatResolveUrlsOnAssigment() && context && u.isRelative() && !u.isEmpty())
                u = context->resolvedUrl(u);
        }
        else if (variantMetaType == QMetaType::fromType<QByteArray>())
            u = QUrl(QString::fromUtf8(value.toByteArray()));
        else if (variantMetaType == QMetaType::fromType<QString>())
            u = QUrl(value.toString());
        else
            return false;

        return property.writeProperty(object, &u, flags);
    } else if (propertyMetaType == QMetaType::fromType<QList<QUrl>>()) {
        QList<QUrl> urlSeq = compatResolveUrlsOnAssigment()
                ? urlSequence(value, context)
                : urlSequence(value);
        return property.writeProperty(object, &urlSeq, flags);
    } else if (property.isQList()) {
        if (propertyMetaType.flags() & QMetaType::IsQmlList) {
            QMetaType listValueType = QQmlMetaType::listValueType(propertyMetaType);
            QQmlMetaObject valueMetaObject = QQmlMetaType::rawMetaObjectForType(listValueType);
            if (valueMetaObject.isNull())
                return false;

            QQmlListProperty<QObject> prop;
            property.readProperty(object, &prop);

            if (!prop.clear || !prop.append)
                return false;

            const bool useNonsignalingListOps = prop.clear == &QQmlVMEMetaObject::list_clear
                    && prop.append == &QQmlVMEMetaObject::list_append;

            auto propClear =
                    useNonsignalingListOps ? &QQmlVMEMetaObject::list_clear_nosignal : prop.clear;
            auto propAppend =
                    useNonsignalingListOps ? &QQmlVMEMetaObject::list_append_nosignal : prop.append;

            propClear(&prop);

            const auto doAppend = [&](QObject *o) {
                if (o && !QQmlMetaObject::canConvert(o, valueMetaObject))
                    o = nullptr;
                propAppend(&prop, o);
            };

            if (variantMetaType == QMetaType::fromType<QQmlListReference>()) {
                QQmlListReference qdlr = value.value<QQmlListReference>();
                for (qsizetype ii = 0; ii < qdlr.count(); ++ii)
                    doAppend(qdlr.at(ii));
            } else if (variantMetaType == QMetaType::fromType<QList<QObject *>>()) {
                const QList<QObject *> &list = qvariant_cast<QList<QObject *> >(value);
                for (qsizetype ii = 0; ii < list.size(); ++ii)
                    doAppend(list.at(ii));
            } else if (variantMetaType == QMetaType::fromType<QList<QVariant>>()) {
                const QList<QVariant> &list
                    = *static_cast<const QList<QVariant> *>(value.constData());
                for (const QVariant &entry : list)
                    doAppend(QQmlMetaType::toQObject(entry));
            } else if (!iterateQObjectContainer(variantMetaType, value.data(), doAppend)) {
                doAppend(QQmlMetaType::toQObject(value));
            }
            if (useNonsignalingListOps) {
                Q_ASSERT(QQmlVMEMetaObject::get(object));
                QQmlVMEResolvedList(&prop).activateSignal();
            }
        } else if (variantMetaType == propertyMetaType) {
            QVariant v = value;
            property.writeProperty(object, v.data(), flags);
        } else {
            QVariant list(propertyMetaType);
            const QQmlType type = QQmlMetaType::qmlType(propertyMetaType);
            const QMetaSequence sequence = type.listMetaSequence();
            if (sequence.canAddValue())
                sequence.addValue(list.data(), value.data());
            property.writeProperty(object, list.data(), flags);
        }
    } else if (enginePriv && propertyMetaType == QMetaType::fromType<QJSValue>()) {
        // We can convert everything into a QJSValue if we have an engine.
        QJSValue jsValue = QJSValuePrivate::fromReturnedValue(
                    enginePriv->v4engine()->metaTypeToJS(variantMetaType, value.constData()));
        return property.writeProperty(object, &jsValue, flags);
    } else {
        Q_ASSERT(variantMetaType != propertyMetaType);

        bool ok = false;
        QVariant v;
        if (variantMetaType == QMetaType::fromType<QString>())
            v = QQmlStringConverters::variantFromString(value.toString(), propertyMetaType, &ok);

        if (!ok) {
            v = value;
            if (v.convert(propertyMetaType)) {
                ok = true;
            }
        }
        if (!ok) {
            // the only other options are that they are assigning a single value
            // or a QVariantList to a sequence type property (eg, an int to a
            // QList<int> property) or that we encountered an interface type.
            // Note that we've already handled single-value assignment to QList<QUrl> properties.
            QSequentialIterable iterable;
            v = QVariant(propertyMetaType);
            if (QMetaType::view(
                        propertyMetaType, v.data(),
                        QMetaType::fromType<QSequentialIterable>(),
                        &iterable)) {
                const QMetaSequence propertyMetaSequence = iterable.metaContainer();
                if (propertyMetaSequence.canAddValueAtEnd()) {
                    const QMetaType elementMetaType = iterable.valueMetaType();
                    void *propertyContainer = iterable.mutableIterable();

                    if (variantMetaType == elementMetaType) {
                        propertyMetaSequence.addValueAtEnd(propertyContainer, value.constData());
                        ok = true;
                    } else if (variantMetaType == QMetaType::fromType<QVariantList>()) {
                        const QVariantList list = value.value<QVariantList>();
                        for (const QVariant &valueElement : list) {
                            if (valueElement.metaType() == elementMetaType) {
                                propertyMetaSequence.addValueAtEnd(
                                            propertyContainer, valueElement.constData());
                            } else {
                                QVariant converted(elementMetaType);
                                QMetaType::convert(
                                            valueElement.metaType(), valueElement.constData(),
                                            elementMetaType, converted.data());
                                propertyMetaSequence.addValueAtEnd(
                                            propertyContainer, converted.constData());
                            }
                        }
                        ok = true;
                    } else if (elementMetaType.flags().testFlag(QMetaType::PointerToQObject)) {
                        const QMetaObject *elementMetaObject = elementMetaType.metaObject();
                        Q_ASSERT(elementMetaObject);

                        const auto doAppend = [&](QObject *o) {
                            QObject *casted = elementMetaObject->cast(o);
                            propertyMetaSequence.addValueAtEnd(propertyContainer, &casted);
                        };

                        if (variantMetaType.flags().testFlag(QMetaType::PointerToQObject)) {
                            doAppend(*static_cast<QObject *const *>(value.data()));
                            ok = true;
                        } else if (variantMetaType == QMetaType::fromType<QQmlListReference>()) {
                            const QQmlListReference *reference
                                    = static_cast<const QQmlListReference *>(value.constData());
                            Q_ASSERT(elementMetaObject);
                            for (int i = 0, end = reference->size(); i < end; ++i)
                                doAppend(reference->at(i));
                            ok = true;
                        } else if (!iterateQObjectContainer(
                                       variantMetaType, value.data(), doAppend)) {
                            doAppend(QQmlMetaType::toQObject(value));
                        }
                    } else {
                        QVariant converted = value;
                        if (converted.convert(elementMetaType)) {
                            propertyMetaSequence.addValueAtEnd(propertyContainer, converted.constData());
                            ok = true;
                        }
                    }
                }
            }
        }

        if (!ok && QQmlMetaType::isInterface(propertyMetaType)) {
            auto valueAsQObject = qvariant_cast<QObject *>(value);

            if (void *iface = valueAsQObject
                        ? valueAsQObject->qt_metacast(QQmlMetaType::interfaceIId(propertyMetaType))
                        : nullptr;
                iface) {
                // this case can occur when object has an interface type
                // and the variant contains a type implementing the interface
                return property.writeProperty(object, &iface, flags);
            }
        }

        if (ok) {
            return property.writeProperty(object, const_cast<void *>(v.constData()), flags);
        } else {
            return false;
        }
    }

    return true;
}

bool QQmlPropertyPrivate::reset(
        QObject *object, const QQmlPropertyData &property,
        QQmlPropertyData::WriteFlags flags)
{
    const BindingFixer bindingFixer(object, property, flags);
    property.resetProperty(object, flags);
    return true;
}

QQmlMetaObject QQmlPropertyPrivate::rawMetaObjectForType(QMetaType metaType)
{
    if (metaType.flags() & QMetaType::PointerToQObject) {
        if (const QMetaObject *metaObject = metaType.metaObject())
            return metaObject;
    }
    return QQmlMetaType::rawMetaObjectForType(metaType);
}

/*!
    Sets the property value to \a value. Returns \c true on success, or
    \c false if the property can't be set because the \a value is the
    wrong type, for example.
 */
bool QQmlProperty::write(const QVariant &value) const
{
    return QQmlPropertyPrivate::write(*this, value, {});
}

/*!
  Writes \a value to the \a name property of \a object.  This method
  is equivalent to:

  \code
    QQmlProperty p(object, name);
    p.write(value);
  \endcode

  Returns \c true on success, \c false otherwise.
*/
bool QQmlProperty::write(QObject *object, const QString &name, const QVariant &value)
{
    QQmlProperty p(object, name);
    return p.write(value);
}

/*!
  Writes \a value to the \a name property of \a object using the
  \l{QQmlContext} {context} \a ctxt.  This method is
  equivalent to:

  \code
    QQmlProperty p(object, name, ctxt);
    p.write(value);
  \endcode

  Returns \c true on success, \c false otherwise.
*/
bool QQmlProperty::write(QObject *object,
                                 const QString &name,
                                 const QVariant &value,
                                 QQmlContext *ctxt)
{
    QQmlProperty p(object, name, ctxt);
    return p.write(value);
}

/*!

  Writes \a value to the \a name property of \a object using the
  environment for instantiating QML components that is provided by
  \a engine.  This method is equivalent to:

  \code
    QQmlProperty p(object, name, engine);
    p.write(value);
  \endcode

  Returns \c true on success, \c false otherwise.
*/
bool QQmlProperty::write(QObject *object, const QString &name, const QVariant &value,
                                 QQmlEngine *engine)
{
    QQmlProperty p(object, name, engine);
    return p.write(value);
}

/*!
    Resets the property and returns true if the property is
    resettable.  If the property is not resettable, nothing happens
    and false is returned.
*/
bool QQmlProperty::reset() const
{
    if (isResettable()) {
        void *args[] = { nullptr };
        QMetaObject::metacall(d->object, QMetaObject::ResetProperty, d->core.coreIndex(), args);
        return true;
    } else {
        return false;
    }
}

bool QQmlPropertyPrivate::write(const QQmlProperty &that,
                                const QVariant &value, QQmlPropertyData::WriteFlags flags)
{
    if (!that.d)
        return false;
    if (that.d->object && that.type() & QQmlProperty::Property &&
        that.d->core.isValid() && that.isWritable())
        return that.d->writeValueProperty(value, flags);
    else
        return false;
}

/*!
    Returns true if the property has a change notifier signal, otherwise false.
*/
bool QQmlProperty::hasNotifySignal() const
{
    if (type() & Property && d->object) {
        return d->object->metaObject()->property(d->core.coreIndex()).hasNotifySignal();
    }
    return false;
}

/*!
    Returns true if the property needs a change notifier signal for bindings
    to remain upto date, false otherwise.

    Some properties, such as attached properties or those whose value never
    changes, do not require a change notifier.
*/
bool QQmlProperty::needsNotifySignal() const
{
    return type() & Property && !property().isConstant();
}

/*!
    Connects the property's change notifier signal to the
    specified \a method of the \a dest object and returns
    true. Returns false if this metaproperty does not
    represent a regular Qt property or if it has no
    change notifier signal, or if the \a dest object does
    not have the specified \a method.
*/
bool QQmlProperty::connectNotifySignal(QObject *dest, int method) const
{
    if (!(type() & Property) || !d->object)
        return false;

    QMetaProperty prop = d->object->metaObject()->property(d->core.coreIndex());
    if (prop.hasNotifySignal()) {
        return QQmlPropertyPrivate::connect(d->object, prop.notifySignalIndex(), dest, method, Qt::DirectConnection);
    } else {
        return false;
    }
}

/*!
    Connects the property's change notifier signal to the
    specified \a slot of the \a dest object and returns
    true. Returns false if this metaproperty does not
    represent a regular Qt property or if it has no
    change notifier signal, or if the \a dest object does
    not have the specified \a slot.

    \note \a slot should be passed using the SLOT() macro so it is
    correctly identified.
*/
bool QQmlProperty::connectNotifySignal(QObject *dest, const char *slot) const
{
    if (!(type() & Property) || !d->object)
        return false;

    QMetaProperty prop = d->object->metaObject()->property(d->core.coreIndex());
    if (prop.hasNotifySignal()) {
        QByteArray signal('2' + prop.notifySignal().methodSignature());
        return QObject::connect(d->object, signal.constData(), dest, slot);
    } else  {
        return false;
    }
}

/*!
    Return the Qt metaobject index of the property.
*/
int QQmlProperty::index() const
{
    return d ? d->core.coreIndex() : -1;
}

QQmlPropertyIndex QQmlPropertyPrivate::propertyIndex(const QQmlProperty &that)
{
    return that.d ? that.d->encodedIndex() : QQmlPropertyIndex();
}

QQmlProperty
QQmlPropertyPrivate::restore(QObject *object, const QQmlPropertyData &data,
                             const QQmlPropertyData *valueTypeData,
                             const QQmlRefPointer<QQmlContextData> &ctxt)
{
    QQmlProperty prop;

    prop.d = new QQmlPropertyPrivate;
    prop.d->object = object;
    prop.d->context = ctxt;
    prop.d->engine = ctxt ? ctxt->engine() : nullptr;

    prop.d->core = data;
    if (valueTypeData)
        prop.d->valueTypeData = *valueTypeData;

    return prop;
}

/*!
    Return the signal corresponding to \a name
*/
QMetaMethod QQmlPropertyPrivate::findSignalByName(const QMetaObject *mo, const QByteArray &name)
{
    Q_ASSERT(mo);
    int methods = mo->methodCount();
    for (int ii = methods - 1; ii >= 2; --ii) { // >= 2 to block the destroyed signal
        QMetaMethod method = mo->method(ii);

        if (method.name() == name && (method.methodType() & QMetaMethod::Signal))
            return method;
    }

    // If no signal is found, but the signal is of the form "onBlahChanged",
    // return the notify signal for the property "Blah"
    if (name.endsWith("Changed")) {
        QByteArray propName = name.mid(0, name.size() - 7);
        int propIdx = mo->indexOfProperty(propName.constData());
        if (propIdx >= 0) {
            QMetaProperty prop = mo->property(propIdx);
            if (prop.hasNotifySignal())
                return prop.notifySignal();
        }
    }

    return QMetaMethod();
}

/*!
    Return the property corresponding to \a name
*/
QMetaProperty QQmlPropertyPrivate::findPropertyByName(const QMetaObject *mo, const QByteArray &name)
{
    Q_ASSERT(mo);
    const int i = mo->indexOfProperty(name);
    return i < 0 ? QMetaProperty() : mo->property(i);
}

/*! \internal
    If \a indexInSignalRange is true, \a index is treated as a signal index
    (see QObjectPrivate::signalIndex()), otherwise it is treated as a
    method index (QMetaMethod::methodIndex()).
*/
static inline void flush_vme_signal(const QObject *object, int index, bool indexInSignalRange)
{
    QQmlData *data = QQmlData::get(object);
    if (data && data->propertyCache) {
        const QQmlPropertyData *property = indexInSignalRange ? data->propertyCache->signal(index)
                                                        : data->propertyCache->method(index);

        if (property && property->isVMESignal()) {
            QQmlVMEMetaObject *vme;
            if (indexInSignalRange)
                vme = QQmlVMEMetaObject::getForSignal(const_cast<QObject *>(object), index);
            else
                vme = QQmlVMEMetaObject::getForMethod(const_cast<QObject *>(object), index);
            vme->connectAliasSignal(index, indexInSignalRange);
        }
    }
}

/*!
Connect \a sender \a signal_index to \a receiver \a method_index with the specified
\a type and \a types.  This behaves identically to QMetaObject::connect() except that
it connects any lazy "proxy" signal connections set up by QML.

It is possible that this logic should be moved to QMetaObject::connect().
*/
bool QQmlPropertyPrivate::connect(const QObject *sender, int signal_index,
                                          const QObject *receiver, int method_index,
                                          int type, int *types)
{
    static const bool indexInSignalRange = false;
    flush_vme_signal(sender, signal_index, indexInSignalRange);
    flush_vme_signal(receiver, method_index, indexInSignalRange);

    return QMetaObject::connect(sender, signal_index, receiver, method_index, type, types);
}

/*! \internal
    \a signal_index MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
void QQmlPropertyPrivate::flushSignal(const QObject *sender, int signal_index)
{
    static const bool indexInSignalRange = true;
    flush_vme_signal(sender, signal_index, indexInSignalRange);
}

QT_END_NAMESPACE

#include "moc_qqmlproperty.cpp"
