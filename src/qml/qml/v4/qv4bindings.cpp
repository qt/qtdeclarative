/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
#include <private/qqmlaccessors_p.h>
#include <private/qqmlprofilerservice_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltrace_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qqmlproperty_p.h>

#include <QtQml/qqmlinfo.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <math.h> // ::fmod

QT_BEGIN_NAMESPACE

using namespace QQmlJS;

namespace {
struct Register {
    typedef QQmlRegisterType Type;

    inline void setUndefined() { dataType = UndefinedType; }
    inline void setNull() { dataType = NullType; }
    inline void setNaN() { setnumber(qSNaN()); }
    inline bool isUndefined() const { return dataType == UndefinedType; }

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

    inline QVariant *getvariantptr() { return (QVariant *)typeDataPtr(); }
    inline QString *getstringptr() { return (QString *)typeDataPtr(); }
    inline QUrl *geturlptr() { return (QUrl *)typeDataPtr(); }
    inline const QVariant *getvariantptr() const { return (QVariant *)typeDataPtr(); }
    inline const QString *getstringptr() const { return (QString *)typeDataPtr(); }
    inline const QUrl *geturlptr() const { return (QUrl *)typeDataPtr(); }

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
};

void Register::cleanup()
{
    if (dataType >= FirstCleanupType) {
        if (dataType == QStringType) {
            getstringptr()->~QString();
        } else if (dataType == QUrlType) {
            geturlptr()->~QUrl();
        } else if (dataType == QColorType) {
            QQml_valueTypeProvider()->destroyValueType(QMetaType::QColor, typeDataPtr(), dataSize());
        } else if (dataType == QVariantType) {
            getvariantptr()->~QVariant();
        }
    }
    setUndefined();
}

void Register::cleanupString()
{
    getstringptr()->~QString();
    setUndefined();
}

void Register::cleanupUrl()
{
    geturlptr()->~QUrl();
    setUndefined();
}

void Register::cleanupColor()
{
    QQml_valueTypeProvider()->destroyValueType(QMetaType::QColor, typeDataPtr(), dataSize());
    setUndefined();
}

void Register::cleanupVariant()
{
    getvariantptr()->~QVariant();
    setUndefined();
}

void Register::copy(const Register &other)
{
    *this = other;
    if (other.dataType >= FirstCleanupType) {
        if (other.dataType == QStringType) 
            new (getstringptr()) QString(*other.getstringptr());
        else if (other.dataType == QUrlType)
            new (geturlptr()) QUrl(*other.geturlptr());
        else if (other.dataType == QColorType)
            QQml_valueTypeProvider()->copyValueType(QMetaType::QColor, other.typeDataPtr(), typeDataPtr(), dataSize());
        else if (other.dataType == QVariantType)
            new (getvariantptr()) QVariant(*other.getvariantptr());
    } 
}

void Register::init(Type type)
{
    dataType = type;
    if (dataType >= FirstCleanupType) {
        if (dataType == QStringType) 
            new (getstringptr()) QString();
        else if (dataType == QUrlType)
            new (geturlptr()) QUrl();
        else if (dataType == QColorType)
            QQml_valueTypeProvider()->initValueType(QMetaType::QColor, typeDataPtr(), dataSize());
        else if (dataType == QVariantType)
            new (getvariantptr()) QVariant();
    }
}

} // end of anonymous namespace

QV4Bindings::QV4Bindings(const char *programData, 
                                               QQmlContextData *context, 
                                               QQmlRefCount *ref)
: subscriptions(0), program(0), dataRef(0), bindings(0)
{
    program = (QV4Program *)programData;
    dataRef = ref;
    if (dataRef) dataRef->addref();

    if (program) {
        subscriptions = new Subscription[program->subscriptions];
        bindings = new Binding[program->bindings];

        QQmlAbstractExpression::setContext(context);
    }
}

QV4Bindings::~QV4Bindings()
{
    delete [] bindings;
    delete [] subscriptions; subscriptions = 0;
    if (dataRef) dataRef->release();
}

QQmlAbstractBinding *QV4Bindings::configBinding(int index, QObject *target, 
                                                        QObject *scope, int property,
                                                        int line, int column)
{
    Binding *rv = bindings + index;

    rv->index = index;
    rv->property = property;
    rv->target = target;
    rv->scope = scope;
    rv->line = line;
    rv->column = column;
    rv->parent = this;

    addref(); // This is decremented in Binding::destroy()

    return rv;
}

void QV4Bindings::Binding::setEnabled(bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    if (enabled != e) {
        enabled = e;

        if (e) update(flags);
    }
}

void QV4Bindings::Binding::update(QQmlPropertyPrivate::WriteFlags flags)
{
    parent->run(this, flags);
}

void QV4Bindings::Binding::destroy()
{
    enabled = false;
    removeFromObject();
    clear();
    removeError();
    parent->release();
}

int QV4Bindings::Binding::propertyIndex() const
{
    //mask out the type information set for value types
    return property & 0xFF00FFFF;
}

QObject *QV4Bindings::Binding::object() const
{
    return target;
}

void QV4Bindings::Subscription::subscriptionCallback(QQmlNotifierEndpoint *e) 
{
    Subscription *s = static_cast<Subscription *>(e);
    s->bindings->subscriptionNotify(s->method);
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
    if (!binding->enabled)
        return;

    QQmlContextData *context = QQmlAbstractExpression::context();
    if (!context || !context->isValid()) 
        return;

    QQmlTrace trace("V4 Binding Update");
    trace.addDetail("URL", context->url);
    trace.addDetail("Line", binding->line);
    trace.addDetail("Column", binding->column);

    QQmlBindingProfiler prof(context->urlString, binding->line, binding->column);

    if (binding->updating) {
        QString name;
        if (binding->property & 0xFFFF0000) {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);

            QQmlValueType *vt = ep->valueTypes[(binding->property >> 16) & 0xFF];
            Q_ASSERT(vt);

            name = QLatin1String(binding->target->metaObject()->property(binding->property & 0xFFFF).name());
            name.append(QLatin1String("."));
            name.append(QLatin1String(vt->metaObject()->property(binding->property >> 24).name()));
        } else {
            name = QLatin1String(binding->target->metaObject()->property(binding->property).name());
        }
        qmlInfo(binding->target) << tr("Binding loop detected for property \"%1\"").arg(name);
        return;
    }

    binding->updating = true;
    if (binding->property & 0xFFFF0000) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);

        QQmlValueType *vt = ep->valueTypes[(binding->property >> 16) & 0xFF];
        Q_ASSERT(vt);
        vt->read(binding->target, binding->property & 0xFFFF);

        QObject *target = vt;
        run(binding->index, binding->executedBlocks, context, binding, binding->scope, target, flags);

        vt->write(binding->target, binding->property & 0xFFFF, flags);
    } else {
        run(binding->index, binding->executedBlocks, context, binding, binding->scope, binding->target, flags);
    }
    binding->updating = false;
}


void QV4Bindings::unsubscribe(int subIndex)
{
    Subscription *sub = (subscriptions + subIndex);
    sub->disconnect();
}

void QV4Bindings::subscribeId(QQmlContextData *p, int idIndex, int subIndex)
{
    unsubscribe(subIndex);

    if (p->idValues[idIndex]) {
        Subscription *sub = (subscriptions + subIndex);
        sub->bindings = this;
        sub->method = subIndex;
        sub->connect(&p->idValues[idIndex].bindings);
    }
}
 
void QV4Bindings::subscribe(QObject *o, int notifyIndex, int subIndex)
{
    Subscription *sub = (subscriptions + subIndex);
    if (sub->isConnected(o, notifyIndex))
        return;
    sub->bindings = this;
    sub->method = subIndex; 
    if (o)
        sub->connect(o, notifyIndex);
    else
        sub->disconnect();
}

static bool testCompareVariants(const QVariant &qtscriptRaw, const QVariant &v4)
{
    QVariant qtscript = qtscriptRaw;

    if (qtscript.userType() == v4.userType()) {
    } else if (qtscript.canConvert((QVariant::Type)v4.userType())) {
        qtscript.convert((QVariant::Type)v4.userType());
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

static void testBindingResult(const QString &binding, int line, int column, 
                              QQmlContextData *context, QObject *scope, 
                              const Register &result, int resultType)
{
    QQmlExpression expression(context->asQQmlContext(), scope, binding);
    bool isUndefined = false;
    QVariant value = expression.evaluate(&isUndefined);

    bool iserror = false;
    QByteArray qtscriptResult;
    QByteArray v4Result;

    if (expression.hasError()) {
        iserror = true;
        qtscriptResult = "exception";
    } else if ((value.userType() != resultType) && (resultType != QMetaType::QVariant)) {
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
        qWarning().nospace() << "QV4: Optimization error @" << context->url.toString().toUtf8().constData() << ":" << line << ":" << column;

        qWarning().nospace() << "    Binding:  " << binding;
        qWarning().nospace() << "    QtScript: " << qtscriptResult.constData();
        qWarning().nospace() << "    V4:       " << v4Result.constData();
    }
}

static void testBindingException(const QString &binding, int line, int column, 
                                 QQmlContextData *context, QObject *scope)
{
    QQmlExpression expression(context->asQQmlContext(), scope, binding);
    bool isUndefined = false;
    QVariant value = expression.evaluate(&isUndefined);

    if (!expression.hasError()) {
        QByteArray qtscriptResult = testResultToString(value, isUndefined);
        qWarning().nospace() << "QV4: Optimization error @" << context->url.toString().toUtf8().constData() << ":" << line << ":" << column;
        qWarning().nospace() << "    Binding:  " << binding;
        qWarning().nospace() << "    QtScript: " << qtscriptResult.constData();
        qWarning().nospace() << "    V4:       exception";
    }
}

static void throwException(int id, QQmlDelayedError *error, 
                           QV4Program *program, QQmlContextData *context,
                           const QString &description = QString())
{
    error->error.setUrl(context->url);
    if (description.isEmpty())
        error->error.setDescription(QLatin1String("TypeError: Result of expression is not an object"));
    else
        error->error.setDescription(description);
    if (id != 0xFF) {
        quint64 e = *((quint64 *)(program->data() + program->exceptionDataOffset) + id); 
        error->error.setLine((e >> 32) & 0xFFFFFFFF);
        error->error.setColumn(e & 0xFFFFFFFF); 
    } else {
        error->error.setLine(-1);
        error->error.setColumn(-1);
    }
    if (!context->engine || !error->addError(QQmlEnginePrivate::get(context->engine)))
        QQmlEnginePrivate::warning(context->engine, error->error);
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

#ifdef QML_THREADED_INTERPRETER
void **QV4Bindings::getDecodeInstrTable()
{
    static void **decode_instr;
    if (!decode_instr) {
        QV4Bindings *dummy = new QV4Bindings(0, 0, 0);
        quint32 executedBlocks = 0;
        dummy->run(0, executedBlocks, 0, 0, 0, 0, 
                   QQmlPropertyPrivate::BypassInterceptor, 
                   &decode_instr);
        dummy->release();
    }
    return decode_instr;
}
#endif

void QV4Bindings::run(int instrIndex, quint32 &executedBlocks,
                                 QQmlContextData *context, QQmlDelayedError *error,
                                 QObject *scope, QObject *output, 
                                 QQmlPropertyPrivate::WriteFlags storeFlags
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

    QML_V4_BEGIN_INSTR(Subscribe, subscribeop)
    {
        QObject *o = 0;
        const Register &object = registers[instr->subscribeop.reg];
        if (!object.isUndefined()) o = object.getQObject();
        subscribe(o, instr->subscribeop.index, instr->subscribeop.offset);
    }
    QML_V4_END_INSTR(Subscribe, subscribeop)

    QML_V4_BEGIN_INSTR(FetchAndSubscribe, fetchAndSubscribe)
    {
        Register &reg = registers[instr->fetchAndSubscribe.reg];

        if (reg.isUndefined()) 
            THROW_EXCEPTION(instr->fetchAndSubscribe.exceptionId);

        QObject *object = reg.getQObject();
        if (!object) {
            reg.setUndefined();
        } else {
            int subIdx = instr->fetchAndSubscribe.subscription;
            Subscription *sub = 0;
            if (subIdx != -1) {
                sub = (subscriptions + subIdx);
                sub->bindings = this;
                sub->method = subIdx;
            }

            const Register::Type valueType = (Register::Type)instr->fetchAndSubscribe.valueType;
            reg.init(valueType);
            if (instr->fetchAndSubscribe.valueType >= FirstCleanupType)
                MARK_REGISTER(instr->fetchAndSubscribe.reg);
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
                if (notifier) sub->connect(notifier);
            } else if (instr->fetchAndSubscribe.property.notifyIndex != -1) {
                sub->connect(object, instr->fetchAndSubscribe.property.notifyIndex);
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

    QML_V4_BEGIN_INSTR(LoadModuleObject, load)
    {
        Register &reg = registers[instr->load.reg];

        const QString *name = reg.getstringptr();
        QQmlTypeNameCache::Result r = context->imports->query(*name);
        reg.cleanupString();

        if (r.isValid() && r.importNamespace) {
            QQmlMetaType::ModuleApiInstance *moduleApi = context->imports->moduleApi(r.importNamespace);
            if (moduleApi) {
                if (moduleApi->qobjectCallback) {
                    moduleApi->qobjectApi = moduleApi->qobjectCallback(context->engine, context->engine);
                    moduleApi->qobjectCallback = 0;
                    moduleApi->scriptCallback = 0;
                }
                if (moduleApi->qobjectApi)
                    reg.setQObject(moduleApi->qobjectApi);
            }
        }
    }
    QML_V4_END_INSTR(LoadModuleObject, load)

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

    QML_V4_BEGIN_INSTR(ConvertIntToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setbool(src.getint());
    }
    QML_V4_END_INSTR(ConvertIntToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
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
            const QString tmp(*src.getstringptr());
            if (instr->unaryop.src == instr->unaryop.output) {
                output.cleanupString();
                MARK_CLEAN_REGISTER(instr->unaryop.output);
            }
            QUrl *urlPtr = output.geturlptr();
            new (urlPtr) QUrl();
            urlPtr->setEncodedUrl(tmp.toUtf8(), QUrl::TolerantMode);

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
    QML_V4_END_INSTR(ConvertStringToUrl, unaryop)

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
        else output.setnumber(qAbs(src.getnumber()));
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
        if (src.isUndefined()) output.setUndefined();
        else output.setint(qFloor(src.getnumber()));
    }
    QML_V4_END_INSTR(MathFloorNumber, unaryop)

    QML_V4_BEGIN_INSTR(MathCeilNumber, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setint(qCeil(src.getnumber()));
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
        if (left.isUndefined() || right.isUndefined()) output.setUndefined();
        else output.setnumber(qMax(left.getnumber(), right.getnumber()));
    }
    QML_V4_END_INSTR(MathMaxNumber, binaryop)

    QML_V4_BEGIN_INSTR(MathMinNumber, binaryop)
    {
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        Register &output = registers[instr->binaryop.output];
        if (left.isUndefined() || right.isUndefined()) output.setUndefined();
        else output.setnumber(qMin(left.getnumber(), right.getnumber()));
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
            const Register::Type valueType = (Register::Type)instr->fetch.valueType;
            reg.init(valueType);
            if (instr->fetch.valueType >= FirstCleanupType)
                MARK_REGISTER(instr->fetch.reg);
            void *argv[] = { reg.typeDataPtr(), 0 };
            QMetaObject::metacall(object, QMetaObject::ReadProperty, instr->fetch.index, argv);
            if (valueType == FloatType) {
                // promote floats
                const double v = reg.getfloat();
                reg.setnumber(v);
            }
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
                const QMetaObject *dataMo = dataObject->metaObject();

                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);
                QMetaProperty receiver = output->metaObject()->property(instr->store.index);
                const QMetaObject *receiverMo = QQmlPropertyPrivate::rawMetaObjectForType(ep, receiver.userType());

                // Verify that these types are compatible
                if (!QQmlPropertyPrivate::canConvert(dataMo, receiverMo)) {
                    THROW_EXCEPTION_STR(instr->store.exceptionId, QLatin1String("Unable to assign ") +
                                                                  QLatin1String(dataMo->className()) +
                                                                  QLatin1String(" to ") +
                                                                  QLatin1String(receiverMo->className()));
                }
            }
        }

        if (instr->store.valueType == FloatType) {
            // cast numbers to floats
            const float v = (float) data.getnumber();
            data.setfloat(v);
        }

        int status = -1;
        void *argv[] = { data.typeDataPtr(), 0, &status, &storeFlags };
        QMetaObject::metacall(output, QMetaObject::WriteProperty,
                              instr->store.index, argv);

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
