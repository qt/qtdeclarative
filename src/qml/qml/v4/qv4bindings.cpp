/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// #define REGISTER_CLEANUP_DEBUG

#include "qv4bindings_p.h"
#include "qv4program_p.h"
#include "qv4compiler_p.h"
#include "qv4compiler_p_p.h"

#include <private/qqmlglobal_p.h>

#include <private/qv8_p.h>
#include <private/qjsconverter_p.h>
#include <private/qjsconverter_impl_p.h>
#include <private/qjsvalue_impl_p.h>
#include <private/qjsvalueiterator_impl_p.h>
#include <private/qv8engine_impl_p.h>

#include <private/qqmlaccessors_p.h>
#include <private/qqmlprofilerservice_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltrace_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include <QtQml/qqmlinfo.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <math.h> // ::fmod

#ifdef Q_CC_MSVC
// MSVC2010 warns about 'unreferenced formal parameter', even if it's used in p->~T()
#  pragma warning( disable : 4100 )
#endif

QT_BEGIN_NAMESPACE

using namespace QQmlJS;

QQmlAbstractBinding::VTable QV4Bindings_Binding_vtable = {
    QV4Bindings::Binding::destroy,
    QQmlAbstractBinding::default_expression,
    QV4Bindings::Binding::propertyIndex,
    QV4Bindings::Binding::object,
    QV4Bindings::Binding::setEnabled,
    QV4Bindings::Binding::update,
    QV4Bindings::Binding::retargetBinding
};

namespace {

// The highest bit is the sign bit, in any endianness
static const quint64 doubleSignMask = (quint64(0x1) << 63);

inline bool signBitSet(const double &v)
{
    union { double d; quint64 u; } u;
    u.d = v;
    return (u.u & doubleSignMask);
}

inline double setSignBit(const double &v, bool b = true)
{
    union { double d; quint64 u; } u;

    u.d = v;
    if (b) {
        u.u |= doubleSignMask;
    } else {
        u.u &= ~doubleSignMask;
    }
    return u.d;
}

inline double clearSignBit(const double &v, bool b = true)
{
    return setSignBit(v, !b);
}

struct Register {
    typedef QQmlRegisterType Type;

    enum SpecialNumericValue {
        NegativeZero = 1,
        PositiveInfinity = 2,
        NegativeInfinity = 3,
        NotANumber = 4
    };

    inline void setUndefined() { dataType = UndefinedType; }
    inline bool isUndefined() const { return dataType == UndefinedType; }

    inline void setNull() { dataType = NullType; }

    inline void setNaN() { setnumber(qSNaN()); }
    inline void setNaNType() { dataType = SpecialNumericType; intValue = NotANumber; } // non-numeric representation of NaN
    inline bool isNaN() const { return (((dataType == SpecialNumericType) && (intValue == NotANumber)) ||
                                        ((dataType == NumberType) && qIsNaN(numberValue))); }

    inline void setInf(bool negative) { setnumber(setSignBit(qInf(), negative)); }
    inline void setInfType(bool negative) { dataType = SpecialNumericType; intValue = (negative ? NegativeInfinity : PositiveInfinity); } // non-numeric representation of Inf
    inline bool isInf() const { return (((dataType == SpecialNumericType) && ((intValue == NegativeInfinity) || (intValue == PositiveInfinity))) ||
                                        ((dataType == NumberType) && qIsInf(numberValue))); }

    inline void setNegativeZero() { setnumber(setSignBit(0)); }
    inline void setNegativeZeroType() { dataType = SpecialNumericType; intValue = NegativeZero; } // non-numeric representation of -0
    inline bool isNegativeZero() const { return (((dataType == SpecialNumericType) && (intValue == NegativeZero)) ||
                                                 ((dataType == NumberType) && (numberValue == 0) && signBitSet(numberValue))); }

    inline void setQObject(QObject *o) { qobjectValue = o; dataType = QObjectStarType; }
    inline QObject *getQObject() const { return qobjectValue; }

    inline void setnumber(double v) { numberValue = v; dataType = NumberType; }
    inline double getnumber() const { return numberValue; }
    inline double &getnumberref() { return numberValue; }

    inline void setfloat(float v) { floatValue = v; dataType = FloatType; }
    inline float getfloat() const { return floatValue; }
    inline float &getfloatref() { return floatValue; }

    inline void setint(int v) { intValue = v; dataType = IntType; }
    inline int getint() const { return intValue; }
    inline int &getintref() { return intValue; }

    inline void setbool(bool v) { boolValue = v; dataType = BoolType; }
    inline bool getbool() const { return boolValue; }
    inline bool &getboolref() { return boolValue; }

    inline QVariant *getvariantptr() { return reinterpret_cast<QVariant *>(typeDataPtr()); }
    inline QString *getstringptr() { return reinterpret_cast<QString *>(typeDataPtr()); }
    inline QUrl *geturlptr() { return reinterpret_cast<QUrl *>(typeDataPtr()); }
    inline v8::Handle<v8::Value> *gethandleptr() { return reinterpret_cast<v8::Handle<v8::Value> *>(typeDataPtr()); }
    inline QJSValue *getjsvalueptr() { return reinterpret_cast<QJSValue *>(typeDataPtr()); }

    inline const QVariant *getvariantptr() const { return reinterpret_cast<const QVariant *>(typeDataPtr()); }
    inline const QString *getstringptr() const { return reinterpret_cast<const QString *>(typeDataPtr()); }
    inline const QUrl *geturlptr() const { return reinterpret_cast<const QUrl *>(typeDataPtr()); }
    inline const v8::Handle<v8::Value> *gethandleptr() const { return reinterpret_cast<const v8::Handle<v8::Value> *>(typeDataPtr()); }
    inline const QJSValue *getjsvalueptr() const { return reinterpret_cast<const QJSValue *>(typeDataPtr()); }

    size_t dataSize() { return sizeof(data); }
    inline void *typeDataPtr() { return (void *)&data; }
    inline void *typeMemory() { return (void *)data; }
    inline const void *typeDataPtr() const { return (void *)&data; }
    inline const void *typeMemory() const { return (void *)data; }

    inline Type gettype() const { return dataType; }
    inline void settype(Type t) { dataType = t; }

    Type dataType;     // Type of data
    union {
        QObject *qobjectValue;
        double numberValue;
        float floatValue;
        int intValue;
        bool boolValue;
        void *data[sizeof(QVariant)];
        qint64 q_for_alignment_1;
        double q_for_alignment_2;
    };

    inline void cleanup();
    inline void cleanupString();
    inline void cleanupUrl();
    inline void cleanupColor();
    inline void cleanupVariant();
    inline void cleanupHandle();
    inline void cleanupJSValue();

    inline void copy(const Register &other);
    inline void init(Type type);
#ifdef REGISTER_CLEANUP_DEBUG
    Register() {
        type = 0;
    }

    ~Register() {
        if (dataType >= FirstCleanupType)
            qWarning("Register leaked of type %d", dataType);
    }
#endif

    template <typename T>
    inline static void copyConstructPointee(T *p, const T *other)
    {
        new (p) T(*other);
    }

    template <typename T>
    inline static void defaultConstructPointee(T *p)
    {
        new (p) T();
    }

    template <typename T>
    inline static void destroyPointee(T *p)
    {
        p->~T();
    }
};

void Register::cleanup()
{
    if (dataType >= FirstCleanupType) {
        if (dataType == QStringType) {
            destroyPointee(getstringptr());
        } else if (dataType == QUrlType) {
            destroyPointee(geturlptr());
        } else if (dataType == QColorType) {
            QQml_valueTypeProvider()->destroyValueType(QMetaType::QColor, typeDataPtr(), dataSize());
        } else if (dataType == QVariantType) {
            destroyPointee(getvariantptr());
        } else if (dataType == qMetaTypeId<v8::Handle<v8::Value> >()) {
            destroyPointee(gethandleptr());
        } else if (dataType == qMetaTypeId<QJSValue>()) {
            destroyPointee(getjsvalueptr());
        }
    }
    setUndefined();
}

void Register::cleanupString()
{
    destroyPointee(getstringptr());
    setUndefined();
}

void Register::cleanupUrl()
{
    destroyPointee(geturlptr());
    setUndefined();
}

void Register::cleanupColor()
{
    QQml_valueTypeProvider()->destroyValueType(QMetaType::QColor, typeDataPtr(), dataSize());
    setUndefined();
}

void Register::cleanupVariant()
{
    destroyPointee(getvariantptr());
    setUndefined();
}

void Register::cleanupHandle()
{
    destroyPointee(gethandleptr());
    setUndefined();
}

void Register::cleanupJSValue()
{
    destroyPointee(getjsvalueptr());
    setUndefined();
}

void Register::copy(const Register &other)
{
    *this = other;
    if (other.dataType >= FirstCleanupType) {
        if (other.dataType == QStringType)
            copyConstructPointee(getstringptr(), other.getstringptr());
        else if (other.dataType == QUrlType)
            copyConstructPointee(geturlptr(), other.geturlptr());
        else if (other.dataType == QColorType)
            QQml_valueTypeProvider()->copyValueType(QMetaType::QColor, other.typeDataPtr(), typeDataPtr(), dataSize());
        else if (other.dataType == QVariantType)
            copyConstructPointee(getvariantptr(), other.getvariantptr());
        else if (other.dataType == qMetaTypeId<v8::Handle<v8::Value> >())
            copyConstructPointee(gethandleptr(), other.gethandleptr());
        else if (other.dataType == qMetaTypeId<QJSValue>())
            copyConstructPointee(getjsvalueptr(), other.getjsvalueptr());
    }
}

void Register::init(Type type)
{
    dataType = type;
    if (dataType >= FirstCleanupType) {
        if (dataType == QStringType)
            defaultConstructPointee(getstringptr());
        else if (dataType == QUrlType)
            defaultConstructPointee(geturlptr());
        else if (dataType == QColorType)
            QQml_valueTypeProvider()->initValueType(QMetaType::QColor, typeDataPtr(), dataSize());
        else if (dataType == QVariantType)
            defaultConstructPointee(getvariantptr());
        else if (dataType == qMetaTypeId<v8::Handle<v8::Value> >())
            defaultConstructPointee(gethandleptr());
        else if (dataType == qMetaTypeId<QJSValue>())
            defaultConstructPointee(getjsvalueptr());
    }
}

} // end of anonymous namespace

QV4Bindings::QV4Bindings(const char *programData, QQmlContextData *context)
: subscriptions(0), program(0), bindings(0)
{
    program = (QV4Program *)programData;
    if (program) {
        subscriptions = new Subscription[program->subscriptions];
        bindings = new Binding[program->bindings];

        QQmlAbstractExpression::setContext(context);
    }
}

QV4Bindings::~QV4Bindings()
{
    delete [] bindings; bindings = 0;
    delete [] subscriptions; subscriptions = 0;
}

QQmlAbstractBinding *QV4Bindings::configBinding(QObject *target, QObject *scope,
                                                const QQmlInstruction::instr_assignV4Binding *i)
{
    Binding *rv = bindings + i->value;

    rv->instruction = i;
    rv->target = target;
    rv->scope = scope;
    rv->parent = this;

    addref(); // This is decremented in Binding::destroy()

    return rv;
}

void QV4Bindings::Binding::setEnabled(QQmlAbstractBinding *_This,
                                      bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    QV4Bindings::Binding *This = static_cast<QV4Bindings::Binding *>(_This);

    if (This->enabledFlag() != e) {
        This->setEnabledFlag(e);

        if (e) update(_This, flags);
    }
}

void QV4Bindings::Binding::update(QQmlAbstractBinding *_This, QQmlPropertyPrivate::WriteFlags flags)
{
    QV4Bindings::Binding *This = static_cast<QV4Bindings::Binding *>(_This);
    This->parent->run(This, flags);
}

void QV4Bindings::Binding::destroy(QQmlAbstractBinding *_This, QQmlAbstractBinding::DestroyMode mode)
{
    QV4Bindings::Binding *This = static_cast<QV4Bindings::Binding *>(_This);

    if (mode == QQmlAbstractBinding::DisconnectBinding)
        This->disconnect();

    This->setEnabledFlag(false);
    This->removeFromObject();
    This->clear();
    This->removeError();
    This->parent->release();
}

int QV4Bindings::Binding::propertyIndex(const QQmlAbstractBinding *_This)
{
    const QV4Bindings::Binding *This = static_cast<const QV4Bindings::Binding *>(_This);

    if (This->target.hasValue()) return This->target.constValue()->targetProperty;
    else return This->instruction->property;
}

QObject *QV4Bindings::Binding::object(const QQmlAbstractBinding *_This)
{
    const QV4Bindings::Binding *This = static_cast<const QV4Bindings::Binding *>(_This);

    if (This->target.hasValue()) return This->target.constValue()->target;
    return *This->target;
}

void QV4Bindings::Binding::retargetBinding(QQmlAbstractBinding *_This, QObject *t, int i)
{
    QV4Bindings::Binding *This = static_cast<QV4Bindings::Binding *>(_This);

    This->target.value().target = t;
    This->target.value().targetProperty = i;
}

void QV4Bindings::Binding::disconnect()
{
    // We iterate over the signal table to find all subscriptions associated with this binding.
    // This is slow, but disconnect() is not called in the common case, only in special cases
    // like when the binding is overwritten.
    QV4Program * const program = parent->program;
    for (quint16 subIndex = 0; subIndex < program->subscriptions; subIndex++) {
        QV4Program::BindingReferenceList * const list = program->signalTable(subIndex);
        for (quint32 bindingIndex = 0; bindingIndex < list->count; ++bindingIndex) {
            QV4Program::BindingReference * const bindingRef = list->bindings + bindingIndex;
            Binding * const binding = parent->bindings + bindingRef->binding;
            if (binding == this) {
                Subscription * const sub = parent->subscriptions + subIndex;
                if (sub->active()) {
                    sub->setActive(false);
                    sub->disconnect();
                }
            }
        }
    }
}

void QV4Bindings::Binding::dump()
{
    qWarning() << parent->context()->url << instruction->line << instruction->column;
}

QV4Bindings::Subscription::Subscription()
    : m_bindings(0)
{
    setCallback(QQmlNotifierEndpoint::QV4BindingsSubscription);
}

int QV4Bindings::Subscription::method() const
{
    Q_ASSERT(bindings() != 0);
    return (this - bindings()->subscriptions);
}

void QV4Bindings::Subscription::setBindings(QV4Bindings *bindings)
{
    m_bindings = bindings;
}

QV4Bindings *QV4Bindings::Subscription::bindings() const
{
    return *m_bindings;
}

bool QV4Bindings::Subscription::active() const
{
    return m_bindings.flag();
}

void QV4Bindings::Subscription::setActive(bool active)
{
    m_bindings.setFlagValue(active);
}

void QV4BindingsSubscription_callback(QQmlNotifierEndpoint *e, void **)
{
    QV4Bindings::Subscription *s = static_cast<QV4Bindings::Subscription *>(e);
    Q_ASSERT(s->bindings());
    s->bindings()->subscriptionNotify(s->method());
}

void QV4Bindings::subscriptionNotify(int id)
{
    QV4Program::BindingReferenceList *list = program->signalTable(id);

    for (quint32 ii = 0; ii < list->count; ++ii) {
        QV4Program::BindingReference *bindingRef = list->bindings + ii;

        Binding *binding = bindings + bindingRef->binding;

        if (binding->executedBlocks & bindingRef->blockMask) {
            run(binding, QQmlPropertyPrivate::DontRemoveBinding);
        }
    }
}

void QV4Bindings::run(Binding *binding, QQmlPropertyPrivate::WriteFlags flags)
{
    if (!binding->enabledFlag())
        return;

    QQmlContextData *context = QQmlAbstractExpression::context();
    if (!context || !context->isValid())
        return;

    // Check that the target has not been deleted
    if (QQmlData::wasDeleted(*binding->target))
        return;

    QQmlTrace trace("V4 Binding Update");
    trace.addDetail("URL", context->url);
    trace.addDetail("Line", binding->instruction->line);
    trace.addDetail("Column", binding->instruction->column);

    QQmlBindingProfiler prof(context->urlString, binding->instruction->line, binding->instruction->column, QQmlProfilerService::V4Binding);

    const int propType = binding->instruction->propType;
    const int property = binding->instruction->property;

    if (binding->updatingFlag()) {
        QString name;
        if (propType) {
            QQmlValueType *vt = QQmlValueTypeFactory::valueType(propType);
            Q_ASSERT(vt);

            name = QLatin1String(binding->target->metaObject()->property(property & 0x0000FFFF).name());
            name.append(QLatin1Char('.'));
            name.append(QLatin1String(vt->metaObject()->property(property >> 16).name()));
        } else {
            name = QLatin1String(binding->target->metaObject()->property(property).name());
        }
        qmlInfo(*binding->target) << tr("Binding loop detected for property \"%1\"").arg(name);
        return;
    }

    int index = binding->instruction->value;
    int fallbackIndex = binding->instruction->fallbackValue;

    bool invalidated = false;
    bool *inv = (fallbackIndex != -1) ? &invalidated : 0;

    binding->setUpdatingFlag(true);
    if (propType) {
        QQmlValueType *vt = QQmlValueTypeFactory::valueType(propType);
        Q_ASSERT(vt);
        vt->read(*binding->target, property & 0x0000FFFF);

        QObject *target = vt;
        run(index, binding->executedBlocks, context, binding, binding->scope, target, flags, inv);

        if (!invalidated) {
            vt->write(*binding->target, property & 0x0000FFFF, flags);
        }
    } else {
        QQmlData *data = QQmlData::get(*binding->target);
        QQmlPropertyData *propertyData = (data && data->propertyCache ? data->propertyCache->property(property) : 0);

        if (propertyData && propertyData->isVarProperty()) {
            // We will allocate a V8 handle in this conversion/store
            v8::HandleScope handle_scope;
            v8::Context::Scope context_scope(QQmlEnginePrivate::get(context->engine)->v8engine()->context());

            run(index, binding->executedBlocks, context, binding, binding->scope, *binding->target, flags, inv);
        } else {
            run(index, binding->executedBlocks, context, binding, binding->scope, *binding->target, flags, inv);
        }
    }
    binding->setUpdatingFlag(false);

    if (invalidated) {
        // This binding is no longer valid - fallback to V8
        Q_ASSERT(fallbackIndex > -1);
        QQmlAbstractBinding *b = QQmlPropertyPrivate::activateSharedBinding(context, fallbackIndex, flags);
        Q_ASSERT(b == binding);
        b->destroy();
    }
}

void QV4Bindings::subscribeId(QQmlContextData *p, int idIndex, int subIndex)
{
    Subscription *sub = (subscriptions + subIndex);
    sub->disconnect();

    if (p->idValues[idIndex]) {
        sub->setBindings(this);
        sub->connect(&p->idValues[idIndex].bindings);
        sub->setActive(true);
    } else {
        sub->setActive(false);
    }
}

void QV4Bindings::subscribe(QObject *o, int notifyIndex, int subIndex, QQmlEngine *e)
{
    Subscription *sub = (subscriptions + subIndex);
    if (sub->isConnected(o, notifyIndex))
        return;
    sub->setBindings(this);
    if (o) {
        sub->connect(o, notifyIndex, e);
        sub->setActive(true);
    } else {
        sub->disconnect();
        sub->setActive(false);
    }
}

static bool testCompareVariants(const QVariant &qtscriptRaw, const QVariant &v4)
{
    QVariant qtscript = qtscriptRaw;

    if (qtscript.userType() == v4.userType()) {
    } else if (qtscript.canConvert(v4.userType())) {
        qtscript.convert(v4.userType());
    } else if (qtscript.userType() == QVariant::Invalid && v4.userType() == QMetaType::QObjectStar) {
        qtscript = qVariantFromValue<QObject *>(0);
    } else {
        return false;
    }

    int type = qtscript.userType();

    if (type == QQmlMetaType::QQuickAnchorLineMetaTypeId()) {
        return QQmlMetaType::QQuickAnchorLineCompare(qtscript.constData(), v4.constData());
    } else if (type == QMetaType::Double) {

        double la = qvariant_cast<double>(qtscript);
        double lr = qvariant_cast<double>(v4);

        return la == lr || (qIsNaN(la) && qIsNaN(lr));

    } else if (type == QMetaType::Float) {

        float la = qvariant_cast<float>(qtscript);
        float lr = qvariant_cast<float>(v4);

        return la == lr || (qIsNaN(la) && qIsNaN(lr));

    } else {
        return qtscript == v4;
    }
}

QByteArray testResultToString(const QVariant &result, bool undefined)
{
    if (undefined) {
        return "undefined";
    } else {
        QString rv;
        QDebug d(&rv);
        d << result;
        return rv.toUtf8();
    }
}

static void testBindingResult(const QString &binding, quint16 line, quint16 column,
                              QQmlContextData *context, QObject *scope, 
                              const Register &result, int resultType)
{
    QQmlExpression expression(context->asQQmlContext(), scope, binding);
    bool isUndefined = false;
    QVariant value = expression.evaluate(&isUndefined);

    bool iserror = false;
    QByteArray qtscriptResult;
    QByteArray v4Result;

    const int handleType = qMetaTypeId<v8::Handle<v8::Value> >();

    if (expression.hasError()) {
        iserror = true;
        qtscriptResult = "exception";
    } else if ((value.userType() != resultType) &&
               (resultType != QMetaType::QVariant) &&
               (resultType != qMetaTypeId<QJSValue>()) &&
               (resultType != handleType)) {
        // Override the QMetaType conversions to make them more JS friendly.
        if (value.userType() == QMetaType::Double && (resultType == QMetaType::QString ||
                                                        resultType == QMetaType::QUrl)) {
            // number to string-like conversion.
            value = QVariant::fromValue<QString>(QString::number(value.toDouble(), 'g', 16));
        } else if (value.userType() == QMetaType::QUrl && resultType == QMetaType::Bool) {
            // url to bool conversion
            value = QVariant::fromValue<bool>(!value.toUrl().isEmpty());
        }

        if (!value.isNull() && !value.convert(resultType)) {
            iserror = true;
            qtscriptResult = "exception";
        } else if (resultType == QMetaType::QUrl) {
            // a V8 value was converted to QUrl.
            value = QVariant::fromValue<QUrl>(context->resolvedUrl(value.toUrl()));
        }
    }

    if (! iserror)
        qtscriptResult = testResultToString(value, isUndefined);

    if (isUndefined && result.isUndefined()) {
        return;
    } else if(isUndefined != result.isUndefined()) {
        iserror = true;
    }

    QVariant v4value;
    if (!result.isUndefined()) {
        switch (resultType) {
        case QMetaType::QString:
            v4value = *result.getstringptr();
            break;
        case QMetaType::QUrl:
            v4value = *result.geturlptr();
            break;
        case QMetaType::QObjectStar:
            v4value = qVariantFromValue<QObject *>(result.getQObject());
            break;
        case QMetaType::Bool:
            v4value = result.getbool();
            break;
        case QMetaType::Int:
            v4value = result.getint();
            break;
        case QMetaType::Double:
            v4value = result.getnumber();
            break;
        case QMetaType::QColor:
            v4value = QVariant(QMetaType::QColor, result.typeDataPtr());
            break;
        case QMetaType::QVariant:
            v4value = *result.getvariantptr();
            break;
        default:
            if (resultType == QQmlMetaType::QQuickAnchorLineMetaTypeId()) {
                v4value = QVariant(QQmlMetaType::QQuickAnchorLineMetaTypeId(), result.typeDataPtr());
            } else if (resultType == qMetaTypeId<QJSValue>()) {
                v4value = result.getjsvalueptr()->toVariant();
            } else if (resultType == handleType) {
                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);
                v4value = ep->v8engine()->toVariant(*result.gethandleptr(), resultType);
            } else {
                iserror = true;
                v4Result = "Unknown V4 type";
            }
        }
    }
    if (v4Result.isEmpty())
        v4Result = testResultToString(v4value, result.isUndefined());

    if (!testCompareVariants(value, v4value))
        iserror = true;

    if (iserror) {
        qWarning().nospace() << "QV4: Optimization error @" << context->url.toString().toUtf8().constData() << ':' << line << ':' << column;

        qWarning().nospace() << "    Binding:  " << binding;
        qWarning().nospace() << "    QtScript: " << qtscriptResult.constData();
        qWarning().nospace() << "    V4:       " << v4Result.constData();
    }
}

static void testBindingException(const QString &binding, quint16 line, quint16 column,
                                 QQmlContextData *context, QObject *scope)
{
    QQmlExpression expression(context->asQQmlContext(), scope, binding);
    bool isUndefined = false;
    QVariant value = expression.evaluate(&isUndefined);

    if (!expression.hasError()) {
        QByteArray qtscriptResult = testResultToString(value, isUndefined);
        qWarning().nospace() << "QV4: Optimization error @" << context->url.toString().toUtf8().constData() << ':' << line << ':' << column;
        qWarning().nospace() << "    Binding:  " << binding;
        qWarning().nospace() << "    QtScript: " << qtscriptResult.constData();
        qWarning().nospace() << "    V4:       exception";
    }
}

static void throwException(int id, QQmlDelayedError *error,
                           QV4Program *program, QQmlContextData *context,
                           const QString &description = QString())
{
    if (description.isEmpty())
        error->setErrorDescription(QLatin1String("TypeError: Result of expression is not an object"));
    else
        error->setErrorDescription(description);
    if (id != 0xFF) {
        quint32 e = *((quint32 *)(program->data() + program->exceptionDataOffset) + id);
        error->setErrorLocation(context->url, (e >> 16), (e & 0xFFFF));
    } else {
        error->setErrorLocation(context->url, -1, -1);
    }
    if (!context->engine || !error->addError(QQmlEnginePrivate::get(context->engine)))
        QQmlEnginePrivate::warning(context->engine, error);
}

const double QV4Bindings::D32 = 4294967296.0;

qint32 QV4Bindings::toInt32(double n)
{
    if (qIsNaN(n) || qIsInf(n) || (n == 0))
        return 0;

    double sign = (n < 0) ? -1.0 : 1.0;
    double abs_n = fabs(n);

    n = ::fmod(sign * ::floor(abs_n), D32);
    const double D31 = D32 / 2.0;

    if (sign == -1 && n < -D31)
        n += D32;

    else if (sign != -1 && n >= D31)
        n -= D32;

    return qint32 (n);
}

inline quint32 QV4Bindings::toUint32(double n)
{
    if (qIsNaN(n) || qIsInf(n) || (n == 0))
        return 0;

    double sign = (n < 0) ? -1.0 : 1.0;
    double abs_n = fabs(n);

    n = ::fmod(sign * ::floor(abs_n), D32);

    if (n < 0)
        n += D32;

    return quint32 (n);
}

#define THROW_EXCEPTION_STR(id, str) { \
    if (testBinding) testBindingException(*testBindingSource, bindingLine, bindingColumn, context, scope); \
    throwException((id), error, program, context, (str)); \
    goto exceptionExit; \
}

#define THROW_VALUE_EXCEPTION_STR(id, str) { \
    throwException((id), error, program, context, (str)); \
    goto exceptionExit; \
}

#define THROW_EXCEPTION(id) THROW_EXCEPTION_STR(id, QString())

#define MARK_REGISTER(reg) cleanupRegisterMask |= (1 << (reg))
#define MARK_CLEAN_REGISTER(reg) cleanupRegisterMask &= ~(1 << (reg))

#define STRING_REGISTER(reg) { \
    registers[(reg)].settype(QStringType); \
    MARK_REGISTER(reg); \
}

#define URL_REGISTER(reg) { \
    registers[(reg)].settype(QUrlType); \
    MARK_REGISTER(reg); \
}

#define COLOR_REGISTER(reg) { \
    registers[(reg)].settype(QColorType); \
    MARK_REGISTER(reg); \
}

#define VARIANT_REGISTER(reg) { \
    registers[(reg)].settype(QVariantType); \
    MARK_REGISTER(reg); \
}

#define V8HANDLE_REGISTER(reg) { \
    registers[(reg)].settype(V8HandleType); \
    MARK_REGISTER(reg); \
}

#define JSVALUE_REGISTER(reg) { \
    registers[(reg)].settype(QJSValueType); \
    MARK_REGISTER(reg); \
}

namespace {

bool bindingInvalidated(bool *invalidated, QObject *obj, QQmlContextData *context, int index)
{
    if (invalidated != 0) {
        if (QQmlData *data = QQmlData::get(obj, true)) {
            if (!data->propertyCache) {
                data->propertyCache = QQmlEnginePrivate::get(context->engine)->cache(obj);
                if (data->propertyCache) data->propertyCache->addref();
            }

            if (QQmlPropertyData *prop = data->propertyCache ? data->propertyCache->property(index) : 0) {
                if (prop->isOverridden()) {
                    // TODO: avoid construction of name and name-based lookup
                    int resolvedIndex = data->propertyCache->property(prop->name(obj), obj, context)->coreIndex;
                    if (index < resolvedIndex) {
                        *invalidated = true;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

}

#ifdef QML_THREADED_INTERPRETER
void **QV4Bindings::getDecodeInstrTable()
{
    static void **decode_instr;
    if (!decode_instr) {
        QV4Bindings *dummy = new QV4Bindings(0, 0);
        quint32 executedBlocks = 0;
        dummy->run(0, executedBlocks, 0, 0, 0, 0, 
                   QQmlPropertyPrivate::BypassInterceptor, 
                   0, &decode_instr);
        dummy->release();
    }
    return decode_instr;
}
#endif

void QV4Bindings::run(int instrIndex, quint32 &executedBlocks,
                                 QQmlContextData *context, QQmlDelayedError *error,
                                 QObject *scope, QObject *output, 
                                 QQmlPropertyPrivate::WriteFlags storeFlags,
                                 bool *invalidated
#ifdef QML_THREADED_INTERPRETER
                                 ,void ***table
#endif
                                 )
{
#ifdef QML_THREADED_INTERPRETER
    if (table) {
        static void *decode_instr[] = {
            FOR_EACH_V4_INSTR(QML_V4_INSTR_ADDR)
        };

        *table = decode_instr;
        return;
    }
#endif


    error->removeError();

    Register registers[32];
    quint32 cleanupRegisterMask = 0;

    executedBlocks = 0;

    const char *code = program->instructions();
    code += instrIndex * QML_V4_INSTR_SIZE(Jump, jump);
    const V4Instr *instr = reinterpret_cast<const V4Instr *>(code);

    const char *data = program->data();

    QString *testBindingSource = 0;
    bool testBinding = false;
    int bindingLine = 0;
    int bindingColumn = 0;

#ifdef QML_THREADED_INTERPRETER
    goto *instr->common.code;
#else
    for (;;) {
        switch (instr->common.type) {
#endif

    QML_V4_BEGIN_INSTR(Noop, common)
    QML_V4_END_INSTR(Noop, common)

    QML_V4_BEGIN_INSTR(BindingId, id)
        bindingLine = instr->id.line;
        bindingColumn = instr->id.column;
    QML_V4_END_INSTR(BindingId, id)

    QML_V4_BEGIN_INSTR(SubscribeId, subscribeop)
        subscribeId(context, instr->subscribeop.index, instr->subscribeop.offset);
    QML_V4_END_INSTR(SubscribeId, subscribeop)

    QML_V4_BEGIN_INSTR(FetchAndSubscribe, fetchAndSubscribe)
    {
        Register &reg = registers[instr->fetchAndSubscribe.reg];

        if (reg.isUndefined())
            THROW_EXCEPTION(instr->fetchAndSubscribe.exceptionId);

        QObject *object = reg.getQObject();
        if (!object) {
            THROW_EXCEPTION(instr->fetchAndSubscribe.exceptionId);
        } else {
            if (bindingInvalidated(invalidated, object, context, instr->fetchAndSubscribe.property.coreIndex))
                goto programExit;

            const Register::Type valueType = (Register::Type)instr->fetchAndSubscribe.valueType;
            reg.init(valueType);
            if (instr->fetchAndSubscribe.valueType >= FirstCleanupType)
                MARK_REGISTER(instr->fetchAndSubscribe.reg);

            QQmlData::flushPendingBinding(object, instr->fetchAndSubscribe.property.coreIndex);

            QQmlAccessors *accessors = instr->fetchAndSubscribe.property.accessors;
            accessors->read(object, instr->fetchAndSubscribe.property.accessorData,
                            reg.typeDataPtr());

            if (valueType == FloatType) {
                // promote floats
                const double v = reg.getfloat();
                reg.setnumber(v);
            }

            if (accessors->notifier) {
                QQmlNotifier *notifier = 0;
                accessors->notifier(object, instr->fetchAndSubscribe.property.accessorData, &notifier);
                if (notifier) {
                    int subIdx = instr->fetchAndSubscribe.subscription;
                    Subscription *sub = 0;
                    if (subIdx != -1) {
                        sub = (subscriptions + subIdx);
                        sub->setBindings(this);
                    }
                    sub->connect(notifier);
                }
            } else {
                const int notifyIndex = instr->fetchAndSubscribe.property.notifyIndex;
                if (notifyIndex != -1) {
                    const int subIdx = instr->fetchAndSubscribe.subscription;
                    subscribe(object, notifyIndex, subIdx, context->engine);
                }
            }
        }
    }
    QML_V4_END_INSTR(FetchAndSubscribe, fetchAndSubscribe)

    QML_V4_BEGIN_INSTR(LoadId, load)
        registers[instr->load.reg].setQObject(context->idValues[instr->load.index].data());
    QML_V4_END_INSTR(LoadId, load)

    QML_V4_BEGIN_INSTR(LoadScope, load)
        registers[instr->load.reg].setQObject(scope);
    QML_V4_END_INSTR(LoadScope, load)

    QML_V4_BEGIN_INSTR(LoadRoot, load)
        registers[instr->load.reg].setQObject(context->contextObject);
    QML_V4_END_INSTR(LoadRoot, load)

    QML_V4_BEGIN_INSTR(LoadSingletonObject, load)
    {
        Register &reg = registers[instr->load.reg];

        const QString *name = reg.getstringptr();
        QQmlTypeNameCache::Result r = context->imports->query(*name);
        reg.cleanupString();

        if (r.isValid() && r.type) {
            if (r.type->isSingleton()) {
                QQmlEngine *e = context->engine;
                QQmlType::SingletonInstanceInfo *siinfo = r.type->singletonInstanceInfo();
                siinfo->init(e); // note: this will also create QJSValue singleton, which is not strictly required here.
                QObject *qobjectSingleton = siinfo->qobjectApi(e);
                if (qobjectSingleton)
                    reg.setQObject(qobjectSingleton);
            }
        }
    }
    QML_V4_END_INSTR(LoadSingletonObject, load)

    QML_V4_BEGIN_INSTR(LoadAttached, attached)
    {
        const Register &input = registers[instr->attached.reg];
        Register &output = registers[instr->attached.output];
        if (input.isUndefined())
            THROW_EXCEPTION(instr->attached.exceptionId);

        QObject *object = registers[instr->attached.reg].getQObject();
        if (!object) {
            output.setUndefined();
        } else {
            QObject *attached = qmlAttachedPropertiesObjectById(instr->attached.id, input.getQObject(), true);
            Q_ASSERT(attached);
            output.setQObject(attached);
        }
    }
    QML_V4_END_INSTR(LoadAttached, attached)

    QML_V4_BEGIN_INSTR(UnaryNot, unaryop)
    {
        registers[instr->unaryop.output].setbool(!registers[instr->unaryop.src].getbool());
    }
    QML_V4_END_INSTR(UnaryNot, unaryop)

    QML_V4_BEGIN_INSTR(UnaryMinusNumber, unaryop)
    {
        registers[instr->unaryop.output].setnumber(-registers[instr->unaryop.src].getnumber());
    }
    QML_V4_END_INSTR(UnaryMinusNumber, unaryop)

    QML_V4_BEGIN_INSTR(UnaryMinusInt, unaryop)
    {
        registers[instr->unaryop.output].setint(-registers[instr->unaryop.src].getint());
    }
    QML_V4_END_INSTR(UnaryMinusInt, unaryop)

    QML_V4_BEGIN_INSTR(UnaryPlusNumber, unaryop)
    {
        registers[instr->unaryop.output].setnumber(+registers[instr->unaryop.src].getnumber());
    }
    QML_V4_END_INSTR(UnaryPlusNumber, unaryop)

    QML_V4_BEGIN_INSTR(UnaryPlusInt, unaryop)
    {
        registers[instr->unaryop.output].setint(+registers[instr->unaryop.src].getint());
    }
    QML_V4_END_INSTR(UnaryPlusInt, unaryop)

    QML_V4_BEGIN_INSTR(ConvertBoolToInt, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setint(src.getbool());
    }
    QML_V4_END_INSTR(ConvertBoolToInt, unaryop)

    QML_V4_BEGIN_INSTR(ConvertBoolToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getjsvalueptr()) QJSValue(src.getbool());
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertBoolToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertBoolToNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setnumber(src.getbool());
    }
    QML_V4_END_INSTR(ConvertBoolToNumber, unaryop)

    QML_V4_BEGIN_INSTR(ConvertBoolToString, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getstringptr()) QString(QLatin1String(src.getbool() ? "true" : "false"));
            STRING_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertBoolToString, unaryop)

    QML_V4_BEGIN_INSTR(ConvertBoolToVariant, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getvariantptr()) QVariant(src.getbool());
            VARIANT_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertBoolToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertBoolToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.gethandleptr()) v8::Handle<v8::Value>(v8::Boolean::New(src.getbool()));
            V8HANDLE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertBoolToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setbool(src.getint());
    }
    QML_V4_END_INSTR(ConvertIntToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getjsvalueptr()) QJSValue(src.getint());
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertIntToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else if (src.isNaN()) output.setNaN();
        else if (src.isInf()) output.setInf(src.getint() == Register::NegativeInfinity);
        else if (src.isNegativeZero()) output.setNegativeZero();
        else output.setnumber(double(src.getint()));
    }
    QML_V4_END_INSTR(ConvertIntToNumber, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToString, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getstringptr()) QString(QString::number(src.getint()));
            STRING_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertIntToString, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToVariant, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getvariantptr()) QVariant(src.getint());
            VARIANT_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertIntToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.gethandleptr()) v8::Handle<v8::Value>(v8::Integer::New(src.getint()));
            V8HANDLE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertIntToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertJSValueToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            QJSValue tmp(*src.getjsvalueptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupJSValue();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            if (tmp.isUndefined()) {
                output.setUndefined();
            } else {
                QV8Engine *v8engine = QQmlEnginePrivate::get(context->engine)->v8engine();
                new (output.gethandleptr()) v8::Handle<v8::Value>(
                        QJSValuePrivate::get(tmp)->asV8Value(v8engine));
                V8HANDLE_REGISTER(instr->unaryop.output);
            }
        }
    }
    QML_V4_END_INSTR(ConvertJSValueToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNumberToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setbool(src.getnumber() != 0);
    }
    QML_V4_END_INSTR(ConvertNumberToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNumberToInt, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setint(toInt32(src.getnumber()));
    }
    QML_V4_END_INSTR(ConvertNumberToInt, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNumberToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getjsvalueptr()) QJSValue(src.getnumber());
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertNumberToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNumberToString, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getstringptr()) QString(QString::number(src.getnumber(), 'g', 16));
            STRING_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertNumberToString, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNumberToVariant, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getvariantptr()) QVariant(src.getnumber());
            VARIANT_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertNumberToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNumberToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.gethandleptr()) v8::Handle<v8::Value>(v8::Number::New(src.getnumber()));
            V8HANDLE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertNumberToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            // Delegate the conversion. This is pretty fast and it doesn't require a QScriptEngine.
            // Ideally we should just call the methods in the QScript namespace directly.
            QJSValue tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            output.setbool(tmp.toBool());
        }
    }
    QML_V4_END_INSTR(ConvertStringToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToInt, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            // Delegate the conversion. This is pretty fast and it doesn't require a QScriptEngine.
            // Ideally we should just call the methods in the QScript namespace directly.
            QJSValue tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            output.setint(tmp.toInt());
        }
    }
    QML_V4_END_INSTR(ConvertStringToInt, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            QString tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.getjsvalueptr()) QJSValue(tmp);
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertStringToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            // Delegate the conversion. This is pretty fast and it doesn't require a QScriptEngine.
            // Ideally we should just call the methods in the QScript namespace directly.
            QJSValue tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            output.setnumber(tmp.toNumber());
        }
    }
    QML_V4_END_INSTR(ConvertStringToNumber, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToUrl, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            QString tmp(*src.getstringptr());
            // Encoded dir-separators defeat QUrl processing - decode them first
            tmp.replace(QLatin1String("%2f"), QLatin1String("/"), Qt::CaseInsensitive);
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.geturlptr()) QUrl(tmp);

            URL_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertStringToUrl, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToColor, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QString tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            QQml_valueTypeProvider()->createValueFromString(QMetaType::QColor, tmp, output.typeDataPtr(), output.dataSize());

            COLOR_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertStringToColor, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToVariant, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QString tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.getvariantptr()) QVariant(tmp);

            VARIANT_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertStringToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QString tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.gethandleptr()) v8::Handle<v8::Value>(QJSConverter::toString(tmp));
            V8HANDLE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertStringToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertUrlToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QUrl tmp(*src.geturlptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupUrl();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            output.setbool(!tmp.isEmpty());
        }
    }
    QML_V4_END_INSTR(ConvertUrlToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertUrlToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QUrl tmp(*src.geturlptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupUrl();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.getjsvalueptr()) QJSValue(tmp.toString());
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertUrlToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertUrlToString, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QUrl tmp(*src.geturlptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupUrl();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.getstringptr()) QString(tmp.toString());
            STRING_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertUrlToString, unaryop)

    QML_V4_BEGIN_INSTR(ConvertUrlToVariant, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QUrl tmp(*src.geturlptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupUrl();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.getvariantptr()) QVariant(tmp);
            VARIANT_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertUrlToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertUrlToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QUrl tmp(*src.geturlptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupUrl();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.gethandleptr()) v8::Handle<v8::Value>(QJSConverter::toString(tmp.toString()));
            V8HANDLE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertUrlToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertColorToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            // for compatibility with color behavior in v8, always true
            output.setbool(true);
        }
    }
    QML_V4_END_INSTR(ConvertColorToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertColorToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QVariant tmp(QMetaType::QColor, src.typeDataPtr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupColor();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }

            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);
            QV8Engine *v8engine = ep->v8engine();
            QQmlValueType *vt = QQmlValueTypeFactory::valueType(QMetaType::QColor);
            v8::HandleScope handle_scope;
            v8::Context::Scope scope(v8engine->context());
            new (output.getjsvalueptr()) QJSValue(v8engine->scriptValueFromInternal(
                    v8engine->valueTypeWrapper()->newValueType(tmp, vt)));
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertColorToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertColorToString, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            QQml_valueTypeProvider()->createStringFromValue(QMetaType::QColor, src.typeDataPtr(), output.getstringptr());
            STRING_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertColorToString, unaryop)

    QML_V4_BEGIN_INSTR(ConvertColorToVariant, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            QVariant tmp(QMetaType::QColor, src.typeDataPtr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupColor();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            new (output.getvariantptr()) QVariant(tmp);
            VARIANT_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertColorToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertColorToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QVariant tmp(QMetaType::QColor, src.typeDataPtr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupColor();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }

            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);
            QQmlValueType *vt = QQmlValueTypeFactory::valueType(QMetaType::QColor);
            new (output.gethandleptr()) v8::Handle<v8::Value>(ep->v8engine()->valueTypeWrapper()->newValueType(tmp, vt));
            V8HANDLE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertColorToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertObjectToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined())
            output.setUndefined();
        else
            output.setbool(src.getQObject() != 0);
    }
    QML_V4_END_INSTR(ConvertObjectToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertObjectToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);
            v8::HandleScope handle_scope;
            v8::Context::Scope scope(ep->v8engine()->context());
            new (output.getjsvalueptr()) QJSValue(context->engine->newQObject(src.getQObject()));
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertObjectToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertObjectToVariant, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined())
            output.setUndefined();
        else {
            new (output.getvariantptr()) QVariant(qVariantFromValue<QObject *>(src.getQObject()));
            VARIANT_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertObjectToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertObjectToVar, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined())
            output.setUndefined();
        else {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);
            new (output.gethandleptr()) v8::Handle<v8::Value>(ep->v8engine()->newQObject(src.getQObject()));
            V8HANDLE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertObjectToVar, unaryop)

    QML_V4_BEGIN_INSTR(ConvertVarToJSValue, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            v8::Handle<v8::Value> tmp(*src.gethandleptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupHandle();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            QV8Engine *v8engine = QQmlEnginePrivate::get(context->engine)->v8engine();
            new (output.getjsvalueptr()) QJSValue(v8engine->scriptValueFromInternal(tmp));
            JSVALUE_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertVarToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNullToJSValue, unaryop)
    {
        Register &output = registers[instr->unaryop.output];
        new (output.getjsvalueptr()) QJSValue(QJSValue::NullValue);
        JSVALUE_REGISTER(instr->unaryop.output);
    }
    QML_V4_END_INSTR(ConvertNullToJSValue, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNullToObject, unaryop)
    {
        Register &output = registers[instr->unaryop.output];
        output.setQObject(0);
    }
    QML_V4_END_INSTR(ConvertNullToObject, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNullToVariant, unaryop)
    {
        Register &output = registers[instr->unaryop.output];
        new (output.getvariantptr()) QVariant();
        VARIANT_REGISTER(instr->unaryop.output);
    }
    QML_V4_END_INSTR(ConvertNullToVariant, unaryop)

    QML_V4_BEGIN_INSTR(ConvertNullToVar, unaryop)
    {
        Register &output = registers[instr->unaryop.output];
        new (output.gethandleptr()) v8::Handle<v8::Value>(v8::Null());
        V8HANDLE_REGISTER(instr->unaryop.output);
    }
    QML_V4_END_INSTR(ConvertNullToVar, unaryop)

    QML_V4_BEGIN_INSTR(ResolveUrl, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            const QUrl tmp(*src.geturlptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                *output.geturlptr() = context->resolvedUrl(tmp);
            } else {
                new (output.geturlptr()) QUrl(context->resolvedUrl(tmp));
                URL_REGISTER(instr->unaryop.output);
            }
        }
    }
    QML_V4_END_INSTR(ResolveUrl, unaryop)

    QML_V4_BEGIN_INSTR(MathSinNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setnumber(qSin(src.getnumber()));
    }
    QML_V4_END_INSTR(MathSinNumber, unaryop)

    QML_V4_BEGIN_INSTR(MathCosNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setnumber(qCos(src.getnumber()));
    }
    QML_V4_END_INSTR(MathCosNumber, unaryop)

    QML_V4_BEGIN_INSTR(MathAbsNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setnumber(clearSignBit(qAbs(src.getnumber())));
    }
    QML_V4_END_INSTR(MathAbsNumber, unaryop)

    QML_V4_BEGIN_INSTR(MathRoundNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setint(qRound(src.getnumber()));
    }
    QML_V4_END_INSTR(MathRoundNumber, unaryop)

    QML_V4_BEGIN_INSTR(MathFloorNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined())
            output.setUndefined();
        else if (src.isNaN())
            // output should be an int, but still NaN
            output.setNaNType();
        else if (src.isInf())
            // output should be an int, but still Inf
            output.setInfType(signBitSet(src.getnumber()));
        else if (src.isNegativeZero())
            // output should be an int, but still -0
            output.setNegativeZeroType();
        else
            output.setint(qFloor(src.getnumber()));
    }
    QML_V4_END_INSTR(MathFloorNumber, unaryop)

    QML_V4_BEGIN_INSTR(MathCeilNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined())
            output.setUndefined();
        else if (src.isNaN())
            // output should be an int, but still NaN
            output.setNaNType();
        else if (src.isInf())
            // output should be an int, but still Inf
            output.setInfType(signBitSet(src.getnumber()));
        else if (src.isNegativeZero())
            // output should be an int, but still -0
            output.setNegativeZeroType();
        else {
            // Ensure that we preserve the sign bit (Math.ceil(-0) -> -0)
            const double input = src.getnumber();
            const int ceiled = qCeil(input);
            if (ceiled == 0 && signBitSet(input)) {
                output.setNegativeZeroType();
            } else {
                output.setint(ceiled);
            }
        }
    }
    QML_V4_END_INSTR(MathCeilNumber, unaryop)

    QML_V4_BEGIN_INSTR(MathPINumber, unaryop)
    {
        static const double qmlPI = 2.0 * qAsin(1.0);
        Register &output = registers[instr->unaryop.output];
        output.setnumber(qmlPI);
    }
    QML_V4_END_INSTR(MathPINumber, unaryop)

    QML_V4_BEGIN_INSTR(LoadNull, null_value)
        registers[instr->null_value.reg].setNull();
    QML_V4_END_INSTR(LoadNull, null_value)

    QML_V4_BEGIN_INSTR(LoadNumber, number_value)
        registers[instr->number_value.reg].setnumber(instr->number_value.value);
    QML_V4_END_INSTR(LoadNumber, number_value)

    QML_V4_BEGIN_INSTR(LoadInt, int_value)
        registers[instr->int_value.reg].setint(instr->int_value.value);
    QML_V4_END_INSTR(LoadInt, int_value)

    QML_V4_BEGIN_INSTR(LoadBool, bool_value)
        registers[instr->bool_value.reg].setbool(instr->bool_value.value);
    QML_V4_END_INSTR(LoadBool, bool_value)

    QML_V4_BEGIN_INSTR(LoadString, string_value)
    {
        Register &output = registers[instr->string_value.reg];
        QChar *string = (QChar *)(data + instr->string_value.offset);
        new (output.getstringptr()) QString(string, instr->string_value.length);
        STRING_REGISTER(instr->string_value.reg);
    }
    QML_V4_END_INSTR(LoadString, string_value)

    QML_V4_BEGIN_INSTR(EnableV4Test, string_value)
    {
        testBindingSource = new QString((QChar *)(data + instr->string_value.offset), instr->string_value.length);
        testBinding = true;
    }
    QML_V4_END_INSTR(String, string_value)

    QML_V4_BEGIN_INSTR(BitAndInt, binaryop)
    {
        registers[instr->binaryop.output].setint(registers[instr->binaryop.left].getint() &
                                                 registers[instr->binaryop.right].getint());
    }
    QML_V4_END_INSTR(BitAndInt, binaryop)

    QML_V4_BEGIN_INSTR(BitOrInt, binaryop)
    {
        registers[instr->binaryop.output].setint(registers[instr->binaryop.left].getint() |
                                                 registers[instr->binaryop.right].getint());
    }
    QML_V4_END_INSTR(BitAndInt, binaryop)

    QML_V4_BEGIN_INSTR(BitXorInt, binaryop)
    {
        registers[instr->binaryop.output].setint(registers[instr->binaryop.left].getint() ^
                                                 registers[instr->binaryop.right].getint());
    }
    QML_V4_END_INSTR(BitXorInt, binaryop)

    QML_V4_BEGIN_INSTR(AddNumber, binaryop)
    {
        registers[instr->binaryop.output].setnumber(registers[instr->binaryop.left].getnumber() +
                                                   registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(AddNumber, binaryop)

    QML_V4_BEGIN_INSTR(AddString, binaryop)
    {
        QString &string = *registers[instr->binaryop.output].getstringptr();
        if (instr->binaryop.output == instr->binaryop.left) {
            string += registers[instr->binaryop.right].getstringptr();
        } else {
            string = *registers[instr->binaryop.left].getstringptr() +
                     *registers[instr->binaryop.right].getstringptr();
        }
    }
    QML_V4_END_INSTR(AddString, binaryop)

    QML_V4_BEGIN_INSTR(SubNumber, binaryop)
    {
        registers[instr->binaryop.output].setnumber(registers[instr->binaryop.left].getnumber() -
                                                   registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(SubNumber, binaryop)

    QML_V4_BEGIN_INSTR(MulNumber, binaryop)
    {
        registers[instr->binaryop.output].setnumber(registers[instr->binaryop.left].getnumber() *
                                                   registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(MulNumber, binaryop)

    QML_V4_BEGIN_INSTR(DivNumber, binaryop)
    {
        registers[instr->binaryop.output].setnumber(registers[instr->binaryop.left].getnumber() /
                                                   registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(DivNumber, binaryop)

    QML_V4_BEGIN_INSTR(ModNumber, binaryop)
    {
        Register &target = registers[instr->binaryop.output];
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        target.setnumber(::fmod(left.getnumber(), right.getnumber()));
    }
    QML_V4_END_INSTR(ModInt, binaryop)

    QML_V4_BEGIN_INSTR(LShiftInt, binaryop)
    {
        registers[instr->binaryop.output].setint(registers[instr->binaryop.left].getint() <<
                                                 registers[instr->binaryop.right].getint());
    }
    QML_V4_END_INSTR(LShiftInt, binaryop)

    QML_V4_BEGIN_INSTR(RShiftInt, binaryop)
    {
        registers[instr->binaryop.output].setint(registers[instr->binaryop.left].getint() >>
                                                 registers[instr->binaryop.right].getint());
    }
    QML_V4_END_INSTR(RShiftInt, binaryop)

    QML_V4_BEGIN_INSTR(URShiftInt, binaryop)
    {
        registers[instr->binaryop.output].setint((unsigned)registers[instr->binaryop.left].getint() >>
                                                 registers[instr->binaryop.right].getint());
    }
    QML_V4_END_INSTR(URShiftInt, binaryop)

    QML_V4_BEGIN_INSTR(GtNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() >
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(GtNumber, binaryop)

    QML_V4_BEGIN_INSTR(LtNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() <
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(LtNumber, binaryop)

    QML_V4_BEGIN_INSTR(GeNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() >=
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(GeNumber, binaryop)

    QML_V4_BEGIN_INSTR(LeNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() <=
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(LeNumber, binaryop)

    QML_V4_BEGIN_INSTR(EqualNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() ==
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(EqualNumber, binaryop)

    QML_V4_BEGIN_INSTR(NotEqualNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() !=
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(NotEqualNumber, binaryop)

    QML_V4_BEGIN_INSTR(StrictEqualNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() ==
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(StrictEqualNumber, binaryop)

    QML_V4_BEGIN_INSTR(StrictNotEqualNumber, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getnumber() !=
                                                  registers[instr->binaryop.right].getnumber());
    }
    QML_V4_END_INSTR(StrictNotEqualNumber, binaryop)

    QML_V4_BEGIN_INSTR(GtString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a > b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(GtString, binaryop)

    QML_V4_BEGIN_INSTR(LtString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a < b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(LtString, binaryop)

    QML_V4_BEGIN_INSTR(GeString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a >= b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(GeString, binaryop)

    QML_V4_BEGIN_INSTR(LeString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a <= b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(LeString, binaryop)

    QML_V4_BEGIN_INSTR(EqualString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a == b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(EqualString, binaryop)

    QML_V4_BEGIN_INSTR(NotEqualString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a != b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(NotEqualString, binaryop)

    QML_V4_BEGIN_INSTR(StrictEqualString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a == b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(StrictEqualString, binaryop)

    QML_V4_BEGIN_INSTR(StrictNotEqualString, binaryop)
    {
        const QString &a = *registers[instr->binaryop.left].getstringptr();
        const QString &b = *registers[instr->binaryop.right].getstringptr();
        bool result = a != b;
        if (instr->binaryop.left == instr->binaryop.output) {
            registers[instr->binaryop.output].cleanupString();
            MARK_CLEAN_REGISTER(instr->binaryop.output);
        }
        registers[instr->binaryop.output].setbool(result);
    }
    QML_V4_END_INSTR(StrictNotEqualString, binaryop)

    QML_V4_BEGIN_INSTR(EqualObject, binaryop)
    {
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        QObject *leftobj = (left.gettype() == NullType) ? 0 : left.getQObject();
        QObject *rightobj = (right.gettype() == NullType) ? 0 : right.getQObject();
        registers[instr->binaryop.output].setbool(leftobj == rightobj);
    }
    QML_V4_END_INSTR(EqualObject, binaryop)

    QML_V4_BEGIN_INSTR(NotEqualObject, binaryop)
    {
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        QObject *leftobj = (left.gettype() == NullType) ? 0 : left.getQObject();
        QObject *rightobj = (right.gettype() == NullType) ? 0 : right.getQObject();
        registers[instr->binaryop.output].setbool(leftobj != rightobj);
    }
    QML_V4_END_INSTR(NotEqualObject, binaryop)

    QML_V4_BEGIN_INSTR(StrictEqualObject, binaryop)
    {
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        QObject *leftobj = (left.gettype() == NullType) ? 0 : left.getQObject();
        QObject *rightobj = (right.gettype() == NullType) ? 0 : right.getQObject();
        registers[instr->binaryop.output].setbool(leftobj == rightobj);
    }
    QML_V4_END_INSTR(StrictEqualObject, binaryop)

    QML_V4_BEGIN_INSTR(StrictNotEqualObject, binaryop)
    {
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        QObject *leftobj = (left.gettype() == NullType) ? 0 : left.getQObject();
        QObject *rightobj = (right.gettype() == NullType) ? 0 : right.getQObject();
        registers[instr->binaryop.output].setbool(leftobj != rightobj);
    }
    QML_V4_END_INSTR(StrictNotEqualObject, binaryop)

    QML_V4_BEGIN_INSTR(MathMaxNumber, binaryop)
    {
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        Register &output = registers[instr->binaryop.output];
        if (left.isUndefined() || right.isUndefined()) {
            output.setUndefined();
        } else {
            const double lhs = left.getnumber();
            const double rhs = right.getnumber();
            double result(lhs);
            if (lhs == rhs) {
                // If these are both zero, +0 is greater than -0
                if (signBitSet(lhs) && !signBitSet(rhs))
                    result = rhs;
            } else {
                result = qMax(lhs, rhs);
            }
            output.setnumber(result);
        }
    }
    QML_V4_END_INSTR(MathMaxNumber, binaryop)

    QML_V4_BEGIN_INSTR(MathMinNumber, binaryop)
    {
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        Register &output = registers[instr->binaryop.output];
        if (left.isUndefined() || right.isUndefined()) {
            output.setUndefined();
        } else {
            const double lhs = left.getnumber();
            const double rhs = right.getnumber();
            double result(lhs);
            if (lhs == rhs) {
                // If these are both zero, -0 is lesser than +0
                if (!signBitSet(lhs) && signBitSet(rhs))
                    result = rhs;
            } else {
                result = qMin(lhs, rhs);
            }
            output.setnumber(result);
        }
    }
    QML_V4_END_INSTR(MathMinNumber, binaryop)

    QML_V4_BEGIN_INSTR(NewString, construct)
    {
        Register &output = registers[instr->construct.reg];
        new (output.getstringptr()) QString;
        STRING_REGISTER(instr->construct.reg);
    }
    QML_V4_END_INSTR(NewString, construct)

    QML_V4_BEGIN_INSTR(NewUrl, construct)
    {
        Register &output = registers[instr->construct.reg];
        new (output.geturlptr()) QUrl;
        URL_REGISTER(instr->construct.reg);
    }
    QML_V4_END_INSTR(NewUrl, construct)

    QML_V4_BEGIN_INSTR(Fetch, fetch)
    {
        Register &reg = registers[instr->fetch.reg];

        if (reg.isUndefined())
            THROW_EXCEPTION(instr->fetch.exceptionId);

        QObject *object = reg.getQObject();
        if (!object) {
            THROW_EXCEPTION(instr->fetch.exceptionId);
        } else {
            if (bindingInvalidated(invalidated, object, context, instr->fetch.index))
                goto programExit;

            const Register::Type valueType = (Register::Type)instr->fetch.valueType;
            reg.init(valueType);
            if (instr->fetch.valueType >= FirstCleanupType)
                MARK_REGISTER(instr->fetch.reg);

            QQmlData::flushPendingBinding(object, instr->fetch.index);

            void *argv[] = { reg.typeDataPtr(), 0 };
            QMetaObject::metacall(object, QMetaObject::ReadProperty, instr->fetch.index, argv);
            if (valueType == FloatType) {
                // promote floats
                const double v = reg.getfloat();
                reg.setnumber(v);
            }

            if (instr->fetch.subIndex != static_cast<quint32>(-1))
                subscribe(object, instr->fetch.subIndex, instr->fetch.subOffset, context->engine);

        }
    }
    QML_V4_END_INSTR(Fetch, fetch)

    QML_V4_BEGIN_INSTR(TestV4Store, storetest)
    {
        Register &data = registers[instr->storetest.reg];
        testBindingResult(*testBindingSource, bindingLine, bindingColumn, context,
                          scope, data, instr->storetest.regType);
    }
    QML_V4_END_INSTR(TestV4Store, storetest)

    QML_V4_BEGIN_INSTR(Store, store)
    {
        Register &data = registers[instr->store.reg];

        if (data.isUndefined())
            THROW_EXCEPTION_STR(instr->store.exceptionId, QLatin1String("Unable to assign undefined value"));

        if (data.gettype() == QObjectStarType) {
            if (QObject *dataObject = data.getQObject()) {
                QQmlMetaObject dataMo(dataObject);

                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);

                QQmlMetaObject receiverMo;

                if (QQmlData::get(output, false) && QQmlData::get(output, false)->propertyCache) {
                    QQmlPropertyData *receiver =
                        QQmlData::get(output, false)->propertyCache->property(instr->store.index);
                    receiverMo = ep->rawMetaObjectForType(receiver->propType);
                } else {
                    QMetaProperty receiver = output->metaObject()->property(instr->store.index);
                    receiverMo = ep->rawMetaObjectForType(receiver.userType());
                }

                // Verify that these types are compatible
                if (!QQmlMetaObject::canConvert(dataMo, receiverMo)) {
                    THROW_EXCEPTION_STR(instr->store.exceptionId, QLatin1String("Unable to assign ") +
                                                                  QLatin1String(dataMo.className()) +
                                                                  QLatin1String(" to ") +
                                                                  QLatin1String(receiverMo.className()));
                }
            }
        }

        if (instr->store.valueType == FloatType) {
            // cast numbers to floats
            const float v = (float) data.getnumber();
            data.setfloat(v);
        }

        if (data.gettype() == V8HandleType) {
            // This property must be a VME var property
            QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(output);
            Q_ASSERT(vmemo);
            vmemo->setVMEProperty(instr->store.index, *data.gethandleptr());
        } else {
            int status = -1;
            void *argv[] = { data.typeDataPtr(), 0, &status, &storeFlags };
            QMetaObject::metacall(output, QMetaObject::WriteProperty,
                                  instr->store.index, argv);
        }

        goto programExit;
    }
    QML_V4_END_INSTR(Store, store)

    QML_V4_BEGIN_INSTR(Copy, copy)
        registers[instr->copy.reg].copy(registers[instr->copy.src]);
        if (registers[instr->copy.reg].gettype() >= FirstCleanupType)
            MARK_REGISTER(instr->copy.reg);
    QML_V4_END_INSTR(Copy, copy)

    QML_V4_BEGIN_INSTR(Jump, jump)
        if (instr->jump.reg == -1 || !registers[instr->jump.reg].getbool())
            code += instr->jump.count;
    QML_V4_END_INSTR(Jump, jump)

    QML_V4_BEGIN_INSTR(BranchTrue, branchop)
        if (registers[instr->branchop.reg].getbool())
            code += instr->branchop.offset;
    QML_V4_END_INSTR(BranchTrue, branchop)

    QML_V4_BEGIN_INSTR(BranchFalse, branchop)
        if (! registers[instr->branchop.reg].getbool())
            code += instr->branchop.offset;
    QML_V4_END_INSTR(BranchFalse, branchop)

    QML_V4_BEGIN_INSTR(Branch, branchop)
        code += instr->branchop.offset;
    QML_V4_END_INSTR(Branch, branchop)

    QML_V4_BEGIN_INSTR(Block, blockop)
        executedBlocks |= instr->blockop.block;
    QML_V4_END_INSTR(Block, blockop)

    QML_V4_BEGIN_INSTR(CleanupRegister, cleanup)
        registers[instr->cleanup.reg].cleanup();
    QML_V4_END_INSTR(CleanupRegister, cleanup)

    QML_V4_BEGIN_INSTR(Throw, throwop)
        THROW_VALUE_EXCEPTION_STR(instr->throwop.exceptionId, *registers[instr->throwop.message].getstringptr());
    QML_V4_END_INSTR(Throw, throwop)

#ifdef QML_THREADED_INTERPRETER
    // nothing to do
#else
    default:
        qFatal("QV4: Unknown instruction %d encountered.", instr->common.type);
        break;
    } // switch

    } // while
#endif

    Q_ASSERT(!"Unreachable code reached");

programExit:
exceptionExit:
    delete testBindingSource;

    int reg = 0;
    while (cleanupRegisterMask) {
        if (cleanupRegisterMask & 0x1)
            registers[reg].cleanup();

        reg++;
        cleanupRegisterMask >>= 1;
    }
}

QT_END_NAMESPACE
