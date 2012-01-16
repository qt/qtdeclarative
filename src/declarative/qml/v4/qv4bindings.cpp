/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

// #define REGISTER_CLEANUP_DEBUG

#include "qv4bindings_p.h"
#include "qv4program_p.h"
#include "qv4compiler_p.h"
#include "qv4compiler_p_p.h"

#include <private/qdeclarativeaccessors_p.h>
#include <private/qdeclarativedebugtrace_p.h>
#include <private/qdeclarativemetatype_p.h>
#include <private/qdeclarativetrace_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <math.h> // ::fmod

QT_BEGIN_NAMESPACE

using namespace QDeclarativeJS;

namespace {
struct Register {
    typedef QDeclarativeRegisterType Type;

    void setUndefined() { dataType = UndefinedType; }
    void setNaN() { setqreal(qSNaN()); }
    bool isUndefined() const { return dataType == UndefinedType; }

    void setQObject(QObject *o) { qobjectValue = o; dataType = QObjectStarType; }
    QObject *getQObject() const { return qobjectValue; }

    void setqreal(qreal v) { qrealValue = v; dataType = QRealType; }
    qreal getqreal() const { return qrealValue; }
    qreal &getqrealref() { return qrealValue; }

    void setint(int v) { intValue = v; dataType = IntType; }
    int getint() const { return intValue; }
    int &getintref() { return intValue; }

    void setbool(bool v) { boolValue = v; dataType = BoolType; }
    bool getbool() const { return boolValue; }
    bool &getboolref() { return boolValue; }

    QVariant *getvariantptr() { return (QVariant *)typeDataPtr(); }
    QString *getstringptr() { return (QString *)typeDataPtr(); }
    QUrl *geturlptr() { return (QUrl *)typeDataPtr(); }
    const QVariant *getvariantptr() const { return (QVariant *)typeDataPtr(); }
    const QString *getstringptr() const { return (QString *)typeDataPtr(); }
    const QUrl *geturlptr() const { return (QUrl *)typeDataPtr(); }

    void *typeDataPtr() { return (void *)&data; }
    void *typeMemory() { return (void *)data; }
    const void *typeDataPtr() const { return (void *)&data; }
    const void *typeMemory() const { return (void *)data; }

    Type gettype() const { return dataType; }
    void settype(Type t) { dataType = t; }

    Type dataType;     // Type of data
    union {
        QObject *qobjectValue;
        qreal qrealValue;
        int intValue;
        bool boolValue;
        void *data[sizeof(QVariant)];
        qint64 q_for_alignment_1;
        double q_for_alignment_2;
    };

    inline void cleanup();
    inline void cleanupString();
    inline void cleanupUrl();
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
        else if (dataType == QVariantType)
            new (getvariantptr()) QVariant();
    }
}

} // end of anonymous namespace

QV4Bindings::QV4Bindings(const char *programData, 
                                               QDeclarativeContextData *context, 
                                               QDeclarativeRefCount *ref)
: subscriptions(0), program(0), dataRef(0), bindings(0)
{
    program = (QV4Program *)programData;
    dataRef = ref;
    if (dataRef) dataRef->addref();

    if (program) {
        subscriptions = new Subscription[program->subscriptions];
        bindings = new Binding[program->bindings];

        QDeclarativeAbstractExpression::setContext(context);
    }
}

QV4Bindings::~QV4Bindings()
{
    delete [] bindings;
    delete [] subscriptions; subscriptions = 0;
    if (dataRef) dataRef->release();
}

QDeclarativeAbstractBinding *QV4Bindings::configBinding(int index, QObject *target, 
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

void QV4Bindings::Binding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (enabled != e) {
        enabled = e;

        if (e) update(flags);
    }
}

void QV4Bindings::Binding::update(QDeclarativePropertyPrivate::WriteFlags flags)
{
    QDeclarativeDebugTrace::startRange(QDeclarativeDebugTrace::Binding);
    if (parent->context())
        QDeclarativeDebugTrace::rangeLocation(QDeclarativeDebugTrace::Binding,
                                              parent->context()->url, line, column);
    parent->run(this, flags);
    QDeclarativeDebugTrace::endRange(QDeclarativeDebugTrace::Binding);
}

void QV4Bindings::Binding::destroy()
{
    enabled = false;
    removeFromObject();
    clear();
    removeError();
    parent->release();
}

void QV4Bindings::Subscription::subscriptionCallback(QDeclarativeNotifierEndpoint *e) 
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
            run(binding, QDeclarativePropertyPrivate::DontRemoveBinding);
        }
    }
}

void QV4Bindings::run(Binding *binding, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (!binding->enabled)
        return;

    QDeclarativeContextData *context = QDeclarativeAbstractExpression::context();
    if (!context || !context->isValid()) 
        return;

    QDeclarativeTrace trace("V4 Binding Update");
    trace.addDetail("URL", context->url);
    trace.addDetail("Line", binding->line);
    trace.addDetail("Column", binding->column);

    if (binding->updating) {
        QString name;
        if (binding->property & 0xFFFF0000) {
            QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context->engine);

            QDeclarativeValueType *vt = ep->valueTypes[(binding->property >> 16) & 0xFF];
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
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context->engine);

        QDeclarativeValueType *vt = ep->valueTypes[(binding->property >> 16) & 0xFF];
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

void QV4Bindings::subscribeId(QDeclarativeContextData *p, int idIndex, int subIndex)
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

// Conversion functions - these MUST match the QtScript expression path
inline static qreal toReal(Register *reg, int type, bool *ok = 0)
{
    if (ok) *ok = true;

    if (type == QMetaType::QReal) {
        return reg->getqreal();
    } else if (type == qMetaTypeId<QVariant>()) {
        return reg->getvariantptr()->toReal();
    } else {
        if (ok) *ok = false;
        return 0;
    }
}

inline static QString toString(Register *reg, int type, bool *ok = 0)
{
    if (ok) *ok = true;

    if (type == QMetaType::QReal) {
        return QString::number(reg->getqreal());
    } else if (type == QMetaType::Int) {
        return QString::number(reg->getint());
    } else if (type == qMetaTypeId<QVariant>()) {
        return reg->getvariantptr()->toString();
    } else if (type == QMetaType::QString) {
        return *reg->getstringptr();
    } else {
        if (ok) *ok = false;
        return QString();
    }
}

inline static bool toBool(Register *reg, int type, bool *ok = 0)
{
    if (ok) *ok = true;

    if (type == QMetaType::Bool) {
        return reg->getbool();
    } else if (type == qMetaTypeId<QVariant>()) {
        return reg->getvariantptr()->toBool();
    } else {
        if (ok) *ok = false;
        return false;
    }
}

inline static QUrl toUrl(Register *reg, int type, QDeclarativeContextData *context, bool *ok = 0)
{
    if (ok) *ok = true;

    QUrl base;
    if (type == qMetaTypeId<QVariant>()) {
        QVariant *var = reg->getvariantptr();
        int vt = var->type();
        if (vt == QVariant::Url) {
            base = var->toUrl();
        } else if (vt == QVariant::ByteArray) {
            // Preserve any valid percent-encoded octets supplied by the source
            base.setEncodedUrl(var->toByteArray(), QUrl::TolerantMode);
        } else if (vt == QVariant::String) {
            base.setEncodedUrl(var->toString().toUtf8(), QUrl::TolerantMode);
        } else {
            if (ok) *ok = false;
            return QUrl();
        }
    } else if (type == QMetaType::QString) {
        base.setEncodedUrl(reg->getstringptr()->toUtf8(), QUrl::TolerantMode);
    } else {
        if (ok) *ok = false;
        return QUrl();
    }

    if (!base.isEmpty() && base.isRelative())
        return context->url.resolved(base);
    else
        return base;
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

    if (type == qMetaTypeId<QDeclarative1AnchorLine>()) {
        QDeclarative1AnchorLine la = qvariant_cast<QDeclarative1AnchorLine>(qtscript);
        QDeclarative1AnchorLine ra = qvariant_cast<QDeclarative1AnchorLine>(v4);

        return la == ra;
    } else if (type == QDeclarativeMetaType::QQuickAnchorLineMetaTypeId()) {
        return QDeclarativeMetaType::QQuickAnchorLineCompare(qtscript.constData(), v4.constData());
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
                              QDeclarativeContextData *context, QObject *scope, 
                              const Register &result, int resultType)
{
    QDeclarativeExpression expression(context->asQDeclarativeContext(), scope, binding);
    bool isUndefined = false;
    QVariant value = expression.evaluate(&isUndefined);

    bool iserror = false;
    QByteArray qtscriptResult;
    QByteArray v4Result;

    if (expression.hasError()) {
        iserror = true;
        qtscriptResult = "exception";
    } else {
        qtscriptResult = testResultToString(value, isUndefined);
    }

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
        case QMetaType::QReal:
            v4value = result.getqreal();
            break;
        default:
            if (resultType == qMetaTypeId<QDeclarative1AnchorLine>()) {
                v4value = qVariantFromValue<QDeclarative1AnchorLine>(*(QDeclarative1AnchorLine *)result.typeDataPtr());
            } else if (resultType == QDeclarativeMetaType::QQuickAnchorLineMetaTypeId()) {
                v4value = QVariant(QDeclarativeMetaType::QQuickAnchorLineMetaTypeId(), result.typeDataPtr());
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
                                 QDeclarativeContextData *context, QObject *scope)
{
    QDeclarativeExpression expression(context->asQDeclarativeContext(), scope, binding);
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

static void throwException(int id, QDeclarativeDelayedError *error, 
                           QV4Program *program, QDeclarativeContextData *context,
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
    if (!context->engine || !error->addError(QDeclarativeEnginePrivate::get(context->engine)))
        QDeclarativeEnginePrivate::warning(context->engine, error->error);
}

const qreal QV4Bindings::D32 = 4294967296.0;

qint32 QV4Bindings::toInt32(qreal n)
{
    if (qIsNaN(n) || qIsInf(n) || (n == 0))
        return 0;

    double sign = (n < 0) ? -1.0 : 1.0;
    qreal abs_n = fabs(n);

    n = ::fmod(sign * ::floor(abs_n), D32);
    const double D31 = D32 / 2.0;

    if (sign == -1 && n < -D31)
        n += D32;

    else if (sign != -1 && n >= D31)
        n -= D32;

    return qint32 (n);
}

inline quint32 QV4Bindings::toUint32(qreal n)
{
    if (qIsNaN(n) || qIsInf(n) || (n == 0))
        return 0;

    double sign = (n < 0) ? -1.0 : 1.0;
    qreal abs_n = fabs(n);

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
                   QDeclarativePropertyPrivate::BypassInterceptor, 
                   &decode_instr);
        dummy->release();
    }
    return decode_instr;
}
#endif

void QV4Bindings::run(int instrIndex, quint32 &executedBlocks,
                                 QDeclarativeContextData *context, QDeclarativeDelayedError *error,
                                 QObject *scope, QObject *output, 
                                 QDeclarativePropertyPrivate::WriteFlags storeFlags
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
            reg.init((Register::Type)instr->fetchAndSubscribe.valueType);
            if (instr->fetchAndSubscribe.valueType >= FirstCleanupType)
                MARK_REGISTER(instr->fetchAndSubscribe.reg);
            QDeclarativeAccessors *accessors = instr->fetchAndSubscribe.property.accessors;
            accessors->read(object, instr->fetchAndSubscribe.property.accessorData,
                            reg.typeDataPtr());

            if (accessors->notifier) {
                QDeclarativeNotifier *notifier = 0;
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

    QML_V4_BEGIN_INSTR(UnaryMinusReal, unaryop)
    {
        registers[instr->unaryop.output].setqreal(-registers[instr->unaryop.src].getqreal());
    }
    QML_V4_END_INSTR(UnaryMinusReal, unaryop)

    QML_V4_BEGIN_INSTR(UnaryMinusInt, unaryop)
    {
        registers[instr->unaryop.output].setint(-registers[instr->unaryop.src].getint());
    }
    QML_V4_END_INSTR(UnaryMinusInt, unaryop)

    QML_V4_BEGIN_INSTR(UnaryPlusReal, unaryop)
    {
        registers[instr->unaryop.output].setqreal(+registers[instr->unaryop.src].getqreal());
    }
    QML_V4_END_INSTR(UnaryPlusReal, unaryop)

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

    QML_V4_BEGIN_INSTR(ConvertBoolToReal, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setqreal(src.getbool());
    }
    QML_V4_END_INSTR(ConvertBoolToReal, unaryop)

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

    QML_V4_BEGIN_INSTR(ConvertIntToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setbool(src.getint());
    }
    QML_V4_END_INSTR(ConvertIntToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertIntToReal, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setqreal(qreal(src.getint()));
    }
    QML_V4_END_INSTR(ConvertIntToReal, unaryop)

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

    QML_V4_BEGIN_INSTR(ConvertRealToBool, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setbool(src.getqreal() != 0);
    }
    QML_V4_END_INSTR(ConvertRealToBool, unaryop)

    QML_V4_BEGIN_INSTR(ConvertRealToInt, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setint(toInt32(src.getqreal()));
    }
    QML_V4_END_INSTR(ConvertRealToInt, unaryop)

    QML_V4_BEGIN_INSTR(ConvertRealToString, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        // ### NaN
        if (src.isUndefined()) {
            output.setUndefined();
        } else {
            new (output.getstringptr()) QString(QString::number(src.getqreal()));
            STRING_REGISTER(instr->unaryop.output);
        }
    }
    QML_V4_END_INSTR(ConvertRealToString, unaryop)

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
            output.setint(tmp.toInt32());
        }
    }
    QML_V4_END_INSTR(ConvertStringToInt, unaryop)

    QML_V4_BEGIN_INSTR(ConvertStringToReal, unaryop)
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
            output.setqreal(tmp.toNumber());
        }
    }
    QML_V4_END_INSTR(ConvertStringToReal, unaryop)

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

    QML_V4_BEGIN_INSTR(MathSinReal, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setqreal(qSin(src.getqreal()));
    }
    QML_V4_END_INSTR(MathSinReal, unaryop)

    QML_V4_BEGIN_INSTR(MathCosReal, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setqreal(qCos(src.getqreal()));
    }
    QML_V4_END_INSTR(MathCosReal, unaryop)

    QML_V4_BEGIN_INSTR(MathRoundReal, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setint(qRound(src.getqreal()));
    }
    QML_V4_END_INSTR(MathRoundReal, unaryop)

    QML_V4_BEGIN_INSTR(MathFloorReal, unaryop)
    {
        const Register &src = registers[instr->unaryop.src];
        Register &output = registers[instr->unaryop.output];
        if (src.isUndefined()) output.setUndefined();
        else output.setint(qFloor(src.getqreal()));
    }
    QML_V4_END_INSTR(MathFloorReal, unaryop)

    QML_V4_BEGIN_INSTR(MathPIReal, unaryop)
    {
        static const qreal qmlPI = 2.0 * qAsin(1.0);
        Register &output = registers[instr->unaryop.output];
        output.setqreal(qmlPI);
    }
    QML_V4_END_INSTR(MathPIReal, unaryop)

    QML_V4_BEGIN_INSTR(LoadReal, real_value)
        registers[instr->real_value.reg].setqreal(instr->real_value.value);
    QML_V4_END_INSTR(LoadReal, real_value)

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

    QML_V4_BEGIN_INSTR(AddReal, binaryop)
    {
        registers[instr->binaryop.output].setqreal(registers[instr->binaryop.left].getqreal() + 
                                                   registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(AddReal, binaryop)

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

    QML_V4_BEGIN_INSTR(SubReal, binaryop)
    {
        registers[instr->binaryop.output].setqreal(registers[instr->binaryop.left].getqreal() - 
                                                   registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(SubReal, binaryop)

    QML_V4_BEGIN_INSTR(MulReal, binaryop)
    {
        registers[instr->binaryop.output].setqreal(registers[instr->binaryop.left].getqreal() * 
                                                   registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(MulReal, binaryop)

    QML_V4_BEGIN_INSTR(DivReal, binaryop)
    {
        registers[instr->binaryop.output].setqreal(registers[instr->binaryop.left].getqreal() / 
                                                   registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(DivReal, binaryop)

    QML_V4_BEGIN_INSTR(ModReal, binaryop)
    {
        Register &target = registers[instr->binaryop.output];
        const Register &left = registers[instr->binaryop.left];
        const Register &right = registers[instr->binaryop.right];
        if (QMetaType::QReal == QMetaType::Float)
            target.setqreal(::fmodf(left.getqreal(), right.getqreal()));
        else
            target.setqreal(::fmod(left.getqreal(), right.getqreal()));
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

    QML_V4_BEGIN_INSTR(GtReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() > 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(GtReal, binaryop)

    QML_V4_BEGIN_INSTR(LtReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() < 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(LtReal, binaryop)

    QML_V4_BEGIN_INSTR(GeReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() >= 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(GeReal, binaryop)

    QML_V4_BEGIN_INSTR(LeReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() <= 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(LeReal, binaryop)

    QML_V4_BEGIN_INSTR(EqualReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() == 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(EqualReal, binaryop)

    QML_V4_BEGIN_INSTR(NotEqualReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() != 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(NotEqualReal, binaryop)

    QML_V4_BEGIN_INSTR(StrictEqualReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() == 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(StrictEqualReal, binaryop)

    QML_V4_BEGIN_INSTR(StrictNotEqualReal, binaryop)
    {
        registers[instr->binaryop.output].setbool(registers[instr->binaryop.left].getqreal() != 
                                                  registers[instr->binaryop.right].getqreal());
    }
    QML_V4_END_INSTR(StrictNotEqualReal, binaryop)

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
            reg.init((Register::Type)instr->fetch.valueType);
            if (instr->fetch.valueType >= FirstCleanupType)
                MARK_REGISTER(instr->fetch.reg);
            void *argv[] = { reg.typeDataPtr(), 0 };
            QMetaObject::metacall(object, QMetaObject::ReadProperty, instr->fetch.index, argv);
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

    // XXX not applicable in v8
    QML_V4_BEGIN_INSTR(InitString, initstring)
//        if (!identifiers[instr->initstring.offset].identifier) {
//            quint32 len = *(quint32 *)(data + instr->initstring.dataIdx);
//            QChar *strdata = (QChar *)(data + instr->initstring.dataIdx + sizeof(quint32));

//            QString str = QString::fromRawData(strdata, len);

//            // identifiers[instr->initstring.offset] = engine->objectClass->createPersistentIdentifier(str);
//        }
    QML_V4_END_INSTR(InitString, initstring)

    QML_V4_BEGIN_INSTR(CleanupRegister, cleanup)
        registers[instr->cleanup.reg].cleanup();
    QML_V4_END_INSTR(CleanupRegister, cleanup)

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
