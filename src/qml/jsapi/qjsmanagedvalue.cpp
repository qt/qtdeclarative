// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qjsmanagedvalue.h>
#include <QtQml/qjsengine.h>
#include <QtQml/private/qv4persistent_p.h>
#include <QtQml/private/qv4engine_p.h>
#include <QtQml/private/qv4mm_p.h>
#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4runtime_p.h>
#include <QtQml/private/qv4functionobject_p.h>
#include <QtQml/private/qv4jscall_p.h>
#include <QtQml/private/qv4urlobject_p.h>
#include <QtQml/private/qv4variantobject_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <QtQml/private/qv4qmetaobjectwrapper_p.h>
#include <QtQml/private/qv4regexpobject_p.h>
#include <QtQml/private/qv4dateobject_p.h>
#include <QtQml/private/qv4errorobject_p.h>
#include <QtQml/private/qv4identifiertable_p.h>

#include <QtCore/qregularexpression.h>
#include <QtCore/qurl.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

/*!
 * \class QJSManagedValue
 * \inmodule QtQml
 * \since 6.1
 *
 * \inmodule QtQml
 *
 * \brief QJSManagedValue represents a value on the JavaScript heap belonging to a QJSEngine.
 *
 * The QJSManagedValue class allows interaction with JavaScript values in most
 * ways you can interact with them from JavaScript itself. You can get and set
 * properties and prototypes, and you can access arrays. Additionally, you can
 * transform the value into the Qt counterparts of JavaScript objects. For
 * example, a Url object may be transformed into a QUrl.
 *
 * A QJSManagedValue is always bound to a particular QJSEngine. You cannot use
 * it independently. This means that you cannot have a QJSManagedValue from one
 * engine be a property or a proptotype of a QJSManagedValue from a different
 * engine.
 *
 * In contrast to QJSValue, almost all values held by QJSManagedValue live on
 * the JavaScript heap. There is no inline or unmanaged storage. Therefore, you
 * can get the prototype of a primitive value, and you can get the \c length
 * property of a string.
 *
 * Only default-constructed or moved-from QJSManagedValues do not hold a value
 * on the JavaScript heap. They represent \c undefined, which doesn't have any
 * properties or prototypes.
 *
 * Also in contrast to QJSValue, QJSManagedValue does not catch any JavaScript
 * exceptions. If an operation on a QJSManagedValue causes an error, it will
 * generally return an \c undefined value and QJSEngine::hasError() will return
 * \c true afterwards. You can then catch the exception using
 * QJSEngine::catchError(), or pass it up the stack, at your own discretion.
 *
 * \note As the reference to the value on the JavaScript heap has to be freed
 * on destruction, you cannot move a QJSManagedValue to a different thread.
 * The destruction would take place in the new thread, which would create a race
 * condition with the garbage collector on the original thread. This also means
 * that you cannot hold a QJSManagedValue beyond the lifespan of its engine.
 *
 * The recommended way of working with a QJSManagedValue is creating it
 * on the stack, possibly by moving a QJSValue and adding an engine, then
 * performing the necessary operations on it, and finally moving it back into a
 * QJSValue for storage. Moving between QJSManagedValue and QJSValue is fast.
 */

/*!
 * \enum QJSManagedValue::Type
 *
 * This enum represents the JavaScript native types, as specified by
 * \l{ECMA-262}.
 *
 * \value Undefined The \c undefined type
 * \value Boolean   The \c boolean type
 * \value Number    The \c number type
 * \value String    The \c string type
 * \value Object    The \c object type
 * \value Symbol    The \c symbol type
 * \value Function  The \c function type
 *
 * Note that the \c null value is not a type of itself but rather a special kind
 * of object. You can query a QJSManagedValue for this condition using the
 * isNull() method. Furthermore, JavaScript has no integer type, but it knows a
 * special treatment of numbers in preparation for integer only operations. You
 * can query a QJSManagedValue to find out whether it holds the result of such a
 * treatment by using the isInteger() method.
 */

/*!
 * \fn QJSManagedValue::QJSManagedValue()
 *
 * Creates a QJSManagedValue that represents the JavaScript \c undefined value.
 * This is the only value not stored on the JavaScript heap. Calling engine()
 * on a default-constructed QJSManagedValue will return nullptr.
 */

static QV4::ExecutionEngine *v4Engine(QV4::Value *d)
{
    if (!d)
        return nullptr;

    QV4::ExecutionEngine *v4 = QV4::PersistentValueStorage::getEngine(d);
    Q_ASSERT(v4);
    return v4;
}

/*!
 * Creates a QJSManagedValue from \a value, using the heap of \a engine. If
 * \a value is itself managed and the engine it belongs to is not \a  engine,
 * the result is an \c undefined value, and a warning is generated.
 */
QJSManagedValue::QJSManagedValue(QJSValue value, QJSEngine *engine)
{
    QV4::ExecutionEngine *v4 = engine->handle();

    if (QV4::Value *m = QJSValuePrivate::takeManagedValue(&value)) {
        if (Q_UNLIKELY(v4Engine(m) != v4)) {
            qWarning("QJSManagedValue(QJSValue, QJSEngine *) failed: "
                     "Value was created in different engine.");
            QV4::PersistentValueStorage::free(m);
            return;
        }

        d = m;
        return;
    }

    d = v4->memoryManager->m_persistentValues->allocate();

    if (const QString *string = QJSValuePrivate::asQString(&value))
        *d = v4->newString(*string);
    else
        *d = QJSValuePrivate::asReturnedValue(&value);
}

/*!
 * Creates a QJSManagedValue from \a value using the heap of \a engine.
 */
QJSManagedValue::QJSManagedValue(const QJSPrimitiveValue &value, QJSEngine *engine) :
    QJSManagedValue(engine->handle())
{
    switch (value.type()) {
    case QJSPrimitiveValue::Undefined:
        *d = QV4::Encode::undefined();
        return;
    case QJSPrimitiveValue::Null:
        *d = QV4::Encode::null();
        return;
    case QJSPrimitiveValue::Boolean:
        *d = QV4::Encode(value.asBoolean());
        return;
    case QJSPrimitiveValue::Integer:
        *d = QV4::Encode(value.asInteger());
        return;
    case QJSPrimitiveValue::Double:
        *d = QV4::Encode(value.asDouble());
        return;
    case QJSPrimitiveValue::String:
        *d = engine->handle()->newString(value.asString());
        return;
    }

    Q_UNREACHABLE();
}

/*!
 * Creates a QJSManagedValue from \a variant using the heap of \a engine.
 */
QJSManagedValue::QJSManagedValue(const QVariant &variant, QJSEngine *engine) :
    QJSManagedValue(engine->handle())
{
    *d = engine->handle()->fromVariant(variant);
}

/*!
 * Creates a QJSManagedValue from \a string using the heap of \a engine.
 */
QJSManagedValue::QJSManagedValue(const QString &string, QJSEngine *engine) :
    QJSManagedValue(engine->handle())
{
    *d = engine->handle()->newString(string);
}

/*!
 * Destroys the QJSManagedValue.
 *
 * \note This frees the memory slot it holds on the JavaScript heap. You must
 *       not destroy a QJSManagedValue from a different thread than the one
 *       where the QJSEngine it belongs to lives.
 */
QJSManagedValue::~QJSManagedValue()
{
    QV4::PersistentValueStorage::free(d);
}

/*!
 * Move-constructs a QJSManagedValue from \a other. This leaves \a other in
 * the default-constructed state where it represents undefined and does not
 * belong to any engine.
 */
QJSManagedValue::QJSManagedValue(QJSManagedValue &&other)
{
    qSwap(d, other.d);
}

/*!
 * Move-assigns a QJSManagedValue from \a other. This leaves \a other in
 * the default-constructed state where it represents undefined and does not
 * belong to any engine.
 *
 * \note This frees the memory slot this QJSManagedValue holds on the
 *       JavaScript heap. You must not move-assign a QJSManagedValue on a
 *       different thread than the one where the QJSEngine it belongs to lives.
 */
QJSManagedValue &QJSManagedValue::operator=(QJSManagedValue &&other)
{
    if (this != &other) {
        QV4::PersistentValueStorage::free(d);
        d = nullptr;
        qSwap(d, other.d);
    }
    return *this;
}

/*!
 * Invokes the JavaScript '==' operator on this QJSManagedValue and \a other,
 * and returns the result.
 *
 * \sa strictlyEquals
 */
bool QJSManagedValue::equals(const QJSManagedValue &other) const
{
    if (!d)
        return !other.d || other.d->isNullOrUndefined();
    if (!other.d)
        return d->isNullOrUndefined();

    return QV4::Runtime::CompareEqual::call(*d, *other.d);
}

/*!
 * Invokes the JavaScript '===' operator on this QJSManagedValue and \a other,
 * and returns the result.
 *
 * \sa equals
 */
bool QJSManagedValue::strictlyEquals(const QJSManagedValue &other) const
{
    if (!d)
        return !other.d || other.d->isUndefined();
    if (!other.d)
        return d->isUndefined();

    return QV4::RuntimeHelpers::strictEqual(*d, *other.d);
}

/*!
 * Returns the QJSEngine this QJSManagedValue belongs to. Mind that the engine
 * is always valid, unless the QJSManagedValue is default-constructed or moved
 * from. In the latter case a nullptr is returned.
 */
QJSEngine *QJSManagedValue::engine() const
{
    if (!d)
        return nullptr;
    if (QV4::ExecutionEngine *v4 = QV4::PersistentValueStorage::getEngine(d))
        return v4->jsEngine();
    return nullptr;
}

/*!
 * Returns the prototype for this QJSManagedValue. This works on any value. You
 * can, for example retrieve the JavaScript \c boolean prototype from a \c boolean
 * value.
 */
QJSManagedValue QJSManagedValue::prototype() const
{
    if (!d)
        return QJSManagedValue();

    QV4::ExecutionEngine *v4 = v4Engine(d);
    QJSManagedValue result(v4);

    if (auto object = d->as<QV4::Object>())
        *result.d = object->getPrototypeOf();
    else if (auto managed = d->as<QV4::Managed>())
        *result.d = managed->internalClass()->prototype;
    else if (d->isBoolean())
        *result.d = v4->booleanPrototype();
    else if (d->isNumber())
        *result.d = v4->numberPrototype();

    // If the prototype appears to be undefined, then it's actually null in JS terms.
    if (result.d->isUndefined())
        *result.d = QV4::Encode::null();

    return result;
}

/*!
 * Sets the prototype of this QJSManagedValue to \a prototype. A precondition
 * is that \a prototype belongs to the same QJSEngine as this QJSManagedValue
 * and is an object (including null). Furthermore, this QJSManagedValue has to
 * be an object (excluding null), too, and you cannot create prototype cycles.
 */
void QJSManagedValue::setPrototype(const QJSManagedValue &prototype)
{
    auto object = d ? d->as<QV4::Object>() : nullptr;
    if (!object) {
        qWarning("QJSManagedValue::setPrototype() failed: "
                 "Can only set a prototype on an object (excluding null).");
        return;
    }

    // Object includes null ...
    if (prototype.type() != QJSManagedValue::Object) {
        qWarning("QJSManagedValue::setPrototype() failed: "
                 "Can only set objects (including null) as prototypes.");
        return;
    }

    if (Q_UNLIKELY(object->engine() != v4Engine(prototype.d))) {
        qWarning("QJSManagedValue::setPrototype() failed: "
                 "Prototype was created in differen engine.");
        return;
    }

    // ... Null becomes nullptr here. That is why it appears as undefined later.
    if (!object->setPrototypeOf(prototype.d->as<QV4::Object>())) {
        qWarning("QJSManagedValue::setPrototype() failed: "
                 "Prototype cycle detected.");
    }
}

/*!
 * Returns the JavaScript type of this QJSManagedValue.
 */
QJSManagedValue::Type QJSManagedValue::type() const
{
    if (!d || d->isUndefined())
        return Undefined;
    if (d->isBoolean())
        return Boolean;
    if (d->isNumber())
        return Number;
    if (d->isString())
        return String;
    if (d->isSymbol())
        return Symbol;
    if (d->isFunctionObject())
        return Function;
    return Object;
}

/*!
 * \fn QJSManagedValue::isUndefined() const
 *
 * Returns \c true if the type of this QJSManagedValue is \c undefined,
 * or \c false otherwise.
 */

/*!
 * \fn QJSManagedValue::isBoolean() const
 *
 * Returns \c true if the type of this QJSManagedValue is \c boolean,
 * or \c false otherwise.
 */

/*!
 * \fn QJSManagedValue::isNumber() const
 *
 * Returns \c true if the type of this QJSManagedValue is \c number,
 * or \c false otherwise.
 */

/*!
 * \fn QJSManagedValue::isString() const
 *
 * Returns \c true if the type of this QJSManagedValue is \c string,
 * or \c false otherwise.
 */

/*!
 * \fn QJSManagedValue::isSymbol() const
 *
 * Returns \c true if the type of this QJSManagedValue is \c symbol,
 * or \c false otherwise.
 */

/*!
 * \fn QJSManagedValue::isObject() const
 *
 * Returns \c true if the type of this QJSManagedValue is \c object,
 * or \c false otherwise.
 */

/*!
 * \fn QJSManagedValue::isFunction() const
 *
 * Returns \c true if the type of this QJSManagedValue is \c function,
 * \c false otherwise.
 */

/*!
 * Returns \c true if this QJSManagedValue holds the JavaScript \c null value,
 * or \c false otherwise.
 */
bool QJSManagedValue::isNull() const
{
    return d && d->isNull();
}

/*!
 * Returns \c true if this QJSManagedValue holds an integer value, or \c false
 * otherwise. The storage format of a number does not affect the result of any
 * operations performed on it, but if an integer is stored, many operations are
 * faster.
 */
bool QJSManagedValue::isInteger() const
{
    return d && d->isInteger();
}

/*!
 * Returns \c true if this value represents a JavaScript regular expression
 * object, or \c false otherwise.
 */
bool QJSManagedValue::isRegularExpression() const
{
    return d && d->as<QV4::RegExpObject>();
}

/*!
 * Returns \c true if this value represents a JavaScript Array
 * object, or \c false otherwise.
 */
bool QJSManagedValue::isArray() const
{
    return d && d->as<QV4::ArrayObject>();
}

/*!
 * Returns \c true if this value represents a JavaScript Url
 * object, or \c false otherwise.
 */
bool QJSManagedValue::isUrl() const
{
    return d && d->as<QV4::UrlObject>();
}

/*!
 * Returns \c true if this value represents a QVariant managed on the JavaScript
 * heap, or \c false otherwise.
 */
bool QJSManagedValue::isVariant() const
{
    return d && d->as<QV4::VariantObject>();
}

/*!
 * Returns \c true if this value represents a QObject pointer managed on the
 * JavaScript heap, or \c false otherwise.
 */
bool QJSManagedValue::isQObject() const
{
    return d && d->as<QV4::QObjectWrapper>();
}

/*!
 * Returns \c true if this value represents a QMetaObject pointer managed on the
 * JavaScript heap, or \c false otherwise.
 */
bool QJSManagedValue::isQMetaObject() const
{
    return d && d->as<QV4::QMetaObjectWrapper>();
}

/*!
 * Returns \c true if this value represents a JavaScript Date object, or
 * \c false otherwise.
 */
bool QJSManagedValue::isDate() const
{
    return d && d->as<QV4::DateObject>();
}

/*!
 * Returns \c true if this value represents a JavaScript Error object, or
 * \c false otherwise.
 */
bool QJSManagedValue::isError() const
{
    return d && d->as<QV4::ErrorObject>();
}

/*!
 * \internal
 *
 * Returns \c true if this value represents a JavaScript meta type, or \c false
 * otherwise.
 */
bool QJSManagedValue::isJsMetaType() const
{
    return d && d->as<QV4::InternalClass>();
}

/*!
 * Converts the manged value to a string. If the managed value holds a string,
 * that one is returned. Otherwise a string coercion by JavaScript rules is
 * performed.
 *
 * \note Conversion of a managed value to a string can throw an exception. In
 *       particular, symbols cannot be coerced into strings, or a custom
 *       toString() method  may throw. In this case the result is an empty
 *       string and the engine carries an error after the conversion.
 */
QString QJSManagedValue::toString() const
{
    return d ? d->toQString() : QStringLiteral("undefined");
}

/*!
 * Converts the manged value to a number. If the managed value holds a number,
 * that one is returned. Otherwise a number coercion by JavaScript rules is
 * performed.
 *
 * \note Conversion of a managed value to a number can throw an exception. In
 *       particular, symbols cannot be coerced into numbers, or a custom
 *       valueOf() method  may throw. In this case the result is 0 and the
 *       engine carries an error after the conversion.
 */
double QJSManagedValue::toNumber() const
{
    return d ? d->toNumber() : 0;
}

/*!
 * Converts the manged value to a boolean. If the managed value holds a boolean,
 * that one is returned. Otherwise a boolean coercion by JavaScript rules is
 * performed.
 */
bool QJSManagedValue::toBoolean() const
{
    return d ? d->toBoolean() : false;
}

/*!
 * Converts the manged value to an integer. This first converts the value to a
 * number by the rules of toNumber(), and then clamps it into the integer range
 * by the rules given for coercing the arguments to JavaScript bit shift
 * operators into 32bit integers.
 *
 * Internally, the value may already be stored as an integer, in which case a
 * fast path is taken.
 *
 * \note Conversion of a managed value to a number can throw an exception. In
 *       particular, symbols cannot be coerced into numbers, or a custom
 *       valueOf() method  may throw. In this case the result is 0 and the
 *       engine carries an error after the conversion.
 *
 * \note The JavaScript rules for coercing numbers into 32bit integers are
 *       unintuitive.
 */
int QJSManagedValue::toInteger() const
{
    return d ? d->toInt32() : 0;
}

/*!
 * Converts the manged value to a QJSPrimitiveValue. If the managed value holds
 * a type supported by QJSPrimitiveValue, the value is copied. Otherwise the
 * value is converted to a string, and the string is stored in
 * QJSPrimitiveValue.
 *
 * \note Conversion of a managed value to a string can throw an exception. In
 *       particular, symbols cannot be coerced into strings, or a custom
 *       toString() method  may throw. In this case the result is the undefined
 *       value and the engine carries an error after the conversion.
 */
QJSPrimitiveValue QJSManagedValue::toPrimitive() const
{
    if (!d || d->isUndefined())
        return QJSPrimitiveUndefined();
    if (d->isNull())
        return QJSPrimitiveNull();
    if (d->isBoolean())
        return d->booleanValue();
    if (d->isInteger())
        return d->integerValue();
    if (d->isNumber())
        return d->doubleValue();

    bool ok;
    const QString result = d->toQString(&ok);
    return ok ? QJSPrimitiveValue(result) : QJSPrimitiveValue(QJSPrimitiveUndefined());
}

/*!
 * Copies this QJSManagedValue into a new QJSValue. This is less efficient than
 * move-constructing a QJSValue from a QJSManagedValue, but retains the
 * QJSManagedValue.
 */
QJSValue QJSManagedValue::toJSValue() const
{
    return d ? QJSValuePrivate::fromReturnedValue(d->asReturnedValue()) : QJSValue();
}

/*!
 * Copies this QJSManagedValue into a new QVariant. This also creates a useful
 * QVariant if QJSManagedValue::isVariant() returns false. QVariant can hold all
 * types supported by QJSManagedValue.
 */
QVariant QJSManagedValue::toVariant() const
{
    if (!d || d->isUndefined())
        return QVariant();
    if (d->isNull())
        return QVariant(QMetaType::fromType<std::nullptr_t>(), nullptr);
    if (d->isBoolean())
        return QVariant(d->booleanValue());
    if (d->isInteger())
        return QVariant(d->integerValue());
    if (d->isNumber())
        return QVariant(d->doubleValue());
    if (d->isString())
        return QVariant(d->toQString());
    if (d->as<QV4::Managed>())
        return QV4::ExecutionEngine::toVariant(*d, QMetaType{}, true);

    Q_UNREACHABLE_RETURN(QVariant());
}

/*!
 * If this QJSManagedValue holds a JavaScript regular expression object, returns
 * an equivalent QRegularExpression. Otherwise returns an invalid one.
 */
QRegularExpression QJSManagedValue::toRegularExpression() const
{
    if (const auto *r = d ? d->as<QV4::RegExpObject>() : nullptr)
        return r->toQRegularExpression();
    return {};
}

/*!
 * If this QJSManagedValue holds a JavaScript Url object, returns
 * an equivalent QUrl. Otherwise returns an invalid one.
 */
QUrl QJSManagedValue::toUrl() const
{
    if (const auto *u = d ? d->as<QV4::UrlObject>() : nullptr)
        return u->toQUrl();
    return {};
}

/*!
 * If this QJSManagedValue holds a QObject pointer, returns it. Otherwise
 * returns nullptr.
 */
QObject *QJSManagedValue::toQObject() const
{
    if (const auto *o = d ? d->as<QV4::QObjectWrapper>() : nullptr)
        return o->object();
    return {};
}

/*!
 * If this QJSManagedValue holds a QMetaObject pointer, returns it.
 * Otherwise returns nullptr.
 */
const QMetaObject *QJSManagedValue::toQMetaObject() const
{
    if (const auto *m = d ? d->as<QV4::QMetaObjectWrapper>() : nullptr)
        return m->metaObject();
    return {};
}

/*!
 * If this QJSManagedValue holds a JavaScript Date object, returns an equivalent
 * QDateTime. Otherwise returns an invalid one.
 */
QDateTime QJSManagedValue::toDateTime() const
{
    if (const auto *t = d ? d->as<QV4::DateObject>() : nullptr)
        return t->toQDateTime();
    return {};
}

/*!
 * Returns \c true if this QJSManagedValue has a property \a name, otherwise
 * returns \c false. The properties of the prototype chain are considered.
 */
bool QJSManagedValue::hasProperty(const QString &name) const
{
    if (!d || d->isNullOrUndefined())
        return false;

    if (d->isString() && name == QStringLiteral("length"))
        return true;

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        QV4::Scope scope(obj->engine());
        QV4::ScopedPropertyKey key(scope, scope.engine->identifierTable->asPropertyKey(name));
        return obj->hasProperty(key);
    }

    return prototype().hasProperty(name);
}

/*!
 * Returns \c true if this QJSManagedValue has a property \a name, otherwise
 * returns \c false. The properties of the prototype chain are not considered.
 */
bool QJSManagedValue::hasOwnProperty(const QString &name) const
{
    if (!d || d->isNullOrUndefined())
        return false;

    if (d->isString() && name == QStringLiteral("length"))
        return true;

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        QV4::Scope scope(obj->engine());
        QV4::ScopedPropertyKey key(scope, scope.engine->identifierTable->asPropertyKey(name));
        return obj->getOwnProperty(key) != QV4::Attr_Invalid;
    }

    return false;
}

/*!
 * Returns the property \a name of this QJSManagedValue. The prototype chain
 * is searched if the property is not found on the actual object.
 */
QJSValue QJSManagedValue::property(const QString &name) const
{
    if (!d)
        return QJSValue();

    if (d->isNullOrUndefined()) {
        QV4::ExecutionEngine *e = v4Engine(d);
        e->throwTypeError(QStringLiteral("Cannot read property '%1' of null").arg(name));
        return QJSValue();
    }

    if (QV4::String *string = d->as<QV4::String>()) {
        if (name == QStringLiteral("length"))
            return QJSValue(string->d()->length());
    }

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        QV4::Scope scope(obj->engine());
        QV4::ScopedPropertyKey key(scope, scope.engine->identifierTable->asPropertyKey(name));
        return QJSValuePrivate::fromReturnedValue(obj->get(key));
    }

    return prototype().property(name);
}

/*!
 * Sets the property \a name to \a value on this QJSManagedValue. This can only
 * be done on JavaScript values of type \c object. Furhermore, \a value has to be
 * either a primitive or belong to the same engine as this value.
 */
void QJSManagedValue::setProperty(const QString &name, const QJSValue &value)
{
    if (!d)
        return;

    if (d->isNullOrUndefined()) {
        v4Engine(d)->throwTypeError(
                    QStringLiteral("Value is null and could not be converted to an object"));
    }

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        QV4::Scope scope(obj->engine());
        QV4::ExecutionEngine *v4 = QJSValuePrivate::engine(&value);
        if (Q_UNLIKELY(v4 && v4 != scope.engine)) {
            qWarning("QJSManagedValue::setProperty() failed: "
                     "Value was created in different engine.");
            return;
        }
        QV4::ScopedPropertyKey key(scope, scope.engine->identifierTable->asPropertyKey(name));
        QV4::ScopedValue val(scope, QJSValuePrivate::convertToReturnedValue(scope.engine, value));
        obj->put(key, val);
    }
}

/*!
 * Deletes the property \a name from this QJSManagedValue. Returns \c true if
 * the deletion succeeded, or \c false otherwise.
 */
bool QJSManagedValue::deleteProperty(const QString &name)
{
    if (!d)
        return false;

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        QV4::Scope scope(obj->engine());
        QV4::ScopedPropertyKey key(scope, scope.engine->identifierTable->asPropertyKey(name));
        return obj->deleteProperty(key);
    }

    return false;
}

/*!
 * Returns \c true if this QJSManagedValue has an array index \a arrayIndex,
 * otherwise returns \c false. The properties of the prototype chain are
 * considered.
 */
bool QJSManagedValue::hasProperty(quint32 arrayIndex) const
{
    if (!d || d->isNullOrUndefined())
        return false;

    if (QV4::String *string = d->as<QV4::String>())
        return arrayIndex < quint32(string->d()->length());

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        bool hasProperty = false;
        if (arrayIndex == std::numeric_limits<quint32>::max())
            obj->get(obj->engine()->id_uintMax(), &hasProperty);
        else
            obj->get(arrayIndex, &hasProperty);
        return hasProperty;
    }

    return prototype().hasProperty(arrayIndex);
}

/*!
 * Returns \c true if this QJSManagedValue has an array index \a arrayIndex,
 * otherwise returns \c false. The properties of the prototype chain are not
 * considered.
 */
bool QJSManagedValue::hasOwnProperty(quint32 arrayIndex) const
{
    if (!d || d->isNullOrUndefined())
        return false;

    if (QV4::String *string = d->as<QV4::String>())
        return arrayIndex < quint32(string->d()->length());

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        if (arrayIndex == std::numeric_limits<quint32>::max()) {
            return obj->getOwnProperty(obj->engine()->id_uintMax()->toPropertyKey())
                    != QV4::Attr_Invalid;
        } else {
            return obj->getOwnProperty(QV4::PropertyKey::fromArrayIndex(arrayIndex))
                    != QV4::Attr_Invalid;
        }
    }

    return false;
}

/*!
 * Returns the property stored at \a arrayIndex of this QJSManagedValue. The
 * prototype chain is searched if the property is not found on the actual
 * object.
 */
QJSValue QJSManagedValue::property(quint32 arrayIndex) const
{
    if (!d || d->isNullOrUndefined())
        return QJSValue();

    if (QV4::String *string = d->as<QV4::String>()) {
        const QString qString = string->toQString();
        if (arrayIndex < quint32(qString.size()))
            return qString.sliced(arrayIndex, 1);
        return QJSValue();
    }

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        if (arrayIndex == std::numeric_limits<quint32>::max())
            return QJSValuePrivate::fromReturnedValue(obj->get(obj->engine()->id_uintMax()));
        else
            return QJSValuePrivate::fromReturnedValue(obj->get(arrayIndex));
    }

    return prototype().property(arrayIndex);
}

/*!
 * Stores the \a value at \a arrayIndex in this QJSManagedValue. This can only
 * be done on JavaScript values of type \c object, and it's not recommended if the
 * value is not an array. Furhermore, \a value has to be either a primitive or
 * belong to the same engine as this value.
 */
void QJSManagedValue::setProperty(quint32 arrayIndex, const QJSValue &value)
{
    if (!d)
        return;

    if (QV4::Object *obj = d->as<QV4::Object>()) {
        QV4::ExecutionEngine *v4 = QJSValuePrivate::engine(&value);
        if (Q_UNLIKELY(v4 && v4 != obj->engine())) {
            qWarning("QJSManagedValue::setProperty() failed: "
                     "Value was created in different engine.");
            return;
        }
        v4 = obj->engine(); // in case value was primitive
        QV4::Scope scope(v4);
        QV4::ScopedValue v(scope, QJSValuePrivate::convertToReturnedValue(v4, value));
        obj->put(arrayIndex, v);
    }
}

/*!
 * Deletes the value stored at \a arrayIndex from this QJSManagedValue. Returns
 * \c true if the deletion succeeded, or \c false otherwise.
 */
bool QJSManagedValue::deleteProperty(quint32 arrayIndex)
{
    if (!d)
        return false;

    if (QV4::Object *obj = d->as<QV4::Object>())
        return obj->deleteProperty(QV4::PropertyKey::fromArrayIndex(arrayIndex));

    return false;
}

static const QV4::FunctionObject *functionObjectForCall(QV4::Value *d)
{
    if (Q_UNLIKELY(!d)) {
        qWarning("QJSManagedValue: Calling a default-constructed or moved-from managed value"
                 "should throw an exception, but there is no engine to receive it.");
        return nullptr;
    }

    if (const QV4::FunctionObject *f = d->as<QV4::FunctionObject>())
        return f;

    v4Engine(d)->throwTypeError(QStringLiteral("Value is not a function"));
    return nullptr;
}

/*!
 * If this QJSManagedValue represents a JavaScript FunctionObject, calls it with
 * the given \a arguments, and returns the result. Otherwise returns a
 * JavaScript \c undefined value.
 *
 * The \a arguments have to be either primitive values or belong to the same
 * QJSEngine as this QJSManagedValue. Otherwise the call is not carried
 * out and a JavaScript \c undefined value is returned.
 */
QJSValue QJSManagedValue::call(const QJSValueList &arguments) const
{
    const QV4::FunctionObject *f = functionObjectForCall(d);
    if (!f)
        return QJSValue();

    QV4::ExecutionEngine *engine = f->engine();

    QV4::Scope scope(engine);
    QV4::JSCallArguments jsCallData(scope, arguments.size());
    *jsCallData.thisObject = engine->globalObject;
    int i = 0;
    for (const QJSValue &arg : arguments) {
        if (Q_UNLIKELY(!QJSValuePrivate::checkEngine(engine, arg))) {
            qWarning("QJSManagedValue::call() failed: Argument was created in different engine.");
            return QJSValue();
        }
        jsCallData.args[i++] = QJSValuePrivate::convertToReturnedValue(engine, arg);
    }

    return QJSValuePrivate::fromReturnedValue(f->call(jsCallData));
}

/*!
 * If this QJSManagedValue represents a JavaScript FunctionObject, calls it on
 * \a instance with the given \a arguments, and returns the result. Otherwise
 * returns a JavaScript \c undefined value.
 *
 * The \a arguments and the \a instance have to be either primitive values or
 * belong to the same QJSEngine as this QJSManagedValue. Otherwise the call is
 * not carried out and a JavaScript \c undefined value is returned.
 */
QJSValue QJSManagedValue::callWithInstance(const QJSValue &instance,
                                           const QJSValueList &arguments) const
{
    const QV4::FunctionObject *f = functionObjectForCall(d);
    if (!f)
        return QJSValue();

    QV4::ExecutionEngine *engine = f->engine();

    if (Q_UNLIKELY(!QJSValuePrivate::checkEngine(engine, instance))) {
        qWarning("QJSManagedValue::callWithInstance() failed: "
                 "Instance was created in different engine.");
        return QJSValue();
    }

    QV4::Scope scope(engine);
    QV4::JSCallArguments jsCallData(scope, arguments.size());
    *jsCallData.thisObject = QJSValuePrivate::convertToReturnedValue(engine, instance);
    int i = 0;
    for (const QJSValue &arg : arguments) {
        if (Q_UNLIKELY(!QJSValuePrivate::checkEngine(engine, arg))) {
            qWarning("QJSManagedValue::callWithInstance() failed: "
                     "Argument was created in different engine.");
            return QJSValue();
        }
        jsCallData.args[i++] = QJSValuePrivate::convertToReturnedValue(engine, arg);
    }

    return QJSValuePrivate::fromReturnedValue(f->call(jsCallData));
}

/*!
 * If this QJSManagedValue represents a JavaScript FunctionObject, calls it as
 * constructor with the given \a arguments, and returns the result. Otherwise
 * returns a JavaScript \c undefined value.
 *
 * The \a arguments have to be either primitive values or belong to the same
 * QJSEngine as this QJSManagedValue. Otherwise the call is not carried
 * out and a JavaScript \c undefined value is returned.
 */
QJSValue QJSManagedValue::callAsConstructor(const QJSValueList &arguments) const
{
    const QV4::FunctionObject *f = functionObjectForCall(d);
    if (!f)
        return QJSValue();

    QV4::ExecutionEngine *engine = f->engine();

    QV4::Scope scope(engine);
    QV4::JSCallArguments jsCallData(scope, arguments.size());
    int i = 0;
    for (const QJSValue &arg : arguments) {
        if (Q_UNLIKELY(!QJSValuePrivate::checkEngine(engine, arg))) {
            qWarning("QJSManagedValue::callAsConstructor() failed: "
                     "Argument was created in different engine.");
            return QJSValue();
        }
        jsCallData.args[i++] = QJSValuePrivate::convertToReturnedValue(engine, arg);
    }

    return QJSValuePrivate::fromReturnedValue(f->callAsConstructor(jsCallData));
}

/*!
 * \internal
 *
 * Retrieves the JavaScript meta type of this value. The JavaScript meta type
 * represents the layout of members in an object. Instantiating a meta type is
 * faster than re-constructing the same object using a sequence of setProperty()
 * calls on a new object.
 *
 * \sa members(), instantiate()
 */
QJSManagedValue QJSManagedValue::jsMetaType() const
{
    if (!d)
        return QJSManagedValue();

    QJSManagedValue result(v4Engine(d));
    if (QV4::Managed *m = d->as<QV4::Managed>())
        *result.d = m->internalClass();

    return result;
}

/*!
 * \internal
 *
 * If this value is a JavaScript meta type, retrieves the names of its members
 * The ordering of the names corresponds to the ordering of the values to be
 * passed to instantiate().
 *
 * If the value is not a meta type, an empty list is returned.
 *
 * \sa isMetaType(), metaType(), instantiate()
 */
QStringList QJSManagedValue::jsMetaMembers() const
{
    if (!d)
        return {};

    if (QV4::InternalClass *c = d->as<QV4::InternalClass>()) {
        const auto heapClass = c->d();
        const int size = heapClass->size;
        QStringList result;
        result.reserve(size);
        QV4::Scope scope(c->engine());
        for (int i = 0; i < size; ++i) {
            QV4::ScopedValue key(scope, heapClass->keyAt(i));
            result.append(key->toQString());
        }
        return result;
    }

    return {};
}

/*!
 * \internal
 *
 * If this value is a JavaScript meta type, instantiates it using the
 * \a values, and returns the result. Otherwise returns undefined.
 *
 * The values are expected in the same order as the keys in the return value of
 * members(), and that is the order in which properties were added to the object
 * this meta type originally belongs to.
 *
 * \sa members(), metaType(), isMetaType().
 */
QJSManagedValue QJSManagedValue::jsMetaInstantiate(const QJSValueList &values) const
{
    if (!d)
        return {};

    if (QV4::InternalClass *c = d->as<QV4::InternalClass>()) {
        QV4::ExecutionEngine *engine = c->engine();
        QJSManagedValue result(engine);
        *result.d = c->engine()->newObject(c->d());
        QV4::Object *o = result.d->as<QV4::Object>();

        QV4::Scope scope(engine);
        QV4::ScopedValue val(scope);
        for (uint i = 0, end = qMin(qsizetype(c->d()->size), values.size()); i < end; ++i) {
            const QJSValue &arg = values[i];
            if (Q_UNLIKELY(!QJSValuePrivate::checkEngine(engine, arg))) {
                qWarning("QJSManagedValue::instantiate() failed: "
                         "Argument was created in different engine.");
                return QJSManagedValue();
            }
            val = QJSValuePrivate::convertToReturnedValue(engine, arg);
            o->setProperty(i, val);
        }

        return result;
    }

    return {};
}

QJSManagedValue::QJSManagedValue(QV4::ExecutionEngine *engine) :
    d(engine->memoryManager->m_persistentValues->allocate())
{
}

QT_END_NAMESPACE
