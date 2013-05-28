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

#include "qqmlvme_p.h"

#include "qqmlcompiler_p.h"
#include "qqmlboundsignal_p.h"
#include "qqmlstringconverters_p.h"
#include <private/qmetaobjectbuilder_p.h>
#include "qqmldata_p.h"
#include "qqml.h"
#include "qqmlinfo.h"
#include "qqmlcustomparser_p.h"
#include "qqmlengine.h"
#include "qqmlcontext.h"
#include "qqmlcomponent.h"
#include "qqmlcomponentattached_p.h"
#include "qqmlbinding_p.h"
#include "qqmlengine_p.h"
#include "qqmlcomponent_p.h"
#include "qqmlvmemetaobject_p.h"
#include "qqmlcontext_p.h"
#include <private/qv4bindings_p.h>
#include <private/qv8bindings_p.h>
#include "qqmlglobal_p.h"
#include <private/qfinitestack_p.h>
#include "qqmlscriptstring.h"
#include "qqmlscriptstring_p.h"
#include "qqmlpropertyvalueinterceptor_p.h"
#include "qqmlvaluetypeproxybinding_p.h"

#include <QStack>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qvarlengtharray.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

using namespace QQmlVMETypes;

#define VME_EXCEPTION(desc, line) \
    { \
        QQmlError error; \
        error.setDescription(desc.trimmed()); \
        error.setLine(line); \
        error.setUrl(COMP->url); \
        *errors << error; \
        goto exceptionExit; \
    }

bool QQmlVME::s_enableComponentComplete = true;

void QQmlVME::init(QQmlContextData *ctxt, QQmlCompiledData *comp, int start,
                           QQmlContextData *creation)
{
    Q_ASSERT(ctxt);
    Q_ASSERT(comp);

    if (start == -1) {
        start = 0;
    } else {
        creationContext = creation;
    }

    State initState;
    initState.context = ctxt;
    initState.compiledData = comp;
    initState.instructionStream = comp->bytecode.constData() + start;
    states.push(initState);

    typedef QQmlInstruction I;
    I *i = (I *)initState.instructionStream;

    Q_ASSERT(comp->instructionType(i) == I::Init);

    objects.allocate(i->init.objectStackSize);
    lists.allocate(i->init.listStackSize);
    bindValues.allocate(i->init.bindingsSize);
    parserStatus.allocate(i->init.parserStatusSize);

#ifdef QML_ENABLE_TRACE
    parserStatusData.allocate(i->init.parserStatusSize);
    rootComponent = comp;
#endif

    rootContext = 0;
    engine = ctxt->engine;
}

bool QQmlVME::initDeferred(QObject *object)
{
    QQmlData *data = QQmlData::get(object);

    if (!data || !data->deferredData)
        return false;

    QQmlContextData *ctxt = data->deferredData->context;
    QQmlCompiledData *comp = data->deferredData->compiledData;
    int start = data->deferredData->deferredIdx;

    State initState;
    initState.flags = State::Deferred;
    initState.context = ctxt;
    initState.compiledData = comp;
    initState.instructionStream = comp->bytecode.constData() + start;
    states.push(initState);

    typedef QQmlInstruction I;
    I *i = (I *)initState.instructionStream;

    Q_ASSERT(comp->instructionType(i) == I::DeferInit);

    objects.allocate(i->deferInit.objectStackSize);
    lists.allocate(i->deferInit.listStackSize);
    bindValues.allocate(i->deferInit.bindingsSize);
    parserStatus.allocate(i->deferInit.parserStatusSize);

    objects.push(object);

#ifdef QML_ENABLE_TRACE
    parserStatusData.allocate(i->deferInit.parserStatusSize);
    rootComponent = comp;
#endif

    rootContext = 0;
    engine = ctxt->engine;

    return true;
}

namespace {
struct ActiveVMERestorer 
{
    ActiveVMERestorer(QQmlVME *me, QQmlEnginePrivate *ep) 
    : ep(ep), oldVME(ep->activeVME) { ep->activeVME = me; }
    ~ActiveVMERestorer() { ep->activeVME = oldVME; }

    QQmlEnginePrivate *ep;
    QQmlVME *oldVME;
};
}

QObject *QQmlVME::execute(QList<QQmlError> *errors, const Interrupt &interrupt)
{
    Q_ASSERT(states.count() >= 1);

#ifdef QML_ENABLE_TRACE
    QQmlTrace trace("VME Execute");
    trace.addDetail("URL", rootComponent->url);
#endif

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(states.at(0).context->engine);

    ActiveVMERestorer restore(this, ep);

    QObject *rv = run(errors, interrupt);

    return rv;
}

inline bool fastHasBinding(QObject *o, int index) 
{
    if (QQmlData *ddata = static_cast<QQmlData *>(QObjectPrivate::get(o)->declarativeData)) {
        int coreIndex = index & 0x0000FFFF;
        return ddata->hasBindingBit(coreIndex);
    }

    return false;
}

static void removeBindingOnProperty(QObject *o, int index)
{
    int coreIndex = index & 0x0000FFFF;
    int valueTypeIndex = (index & 0xFFFF0000 ? index >> 16 : -1);

    QQmlAbstractBinding *binding = QQmlPropertyPrivate::setBinding(o, coreIndex, valueTypeIndex, 0);
    if (binding) binding->destroy();
}

static QVariant variantFromString(const QString &string)
{
    return QQmlStringConverters::variantFromString(string);
}

// XXX we probably need some form of "work count" here to prevent us checking this 
// for every instruction.
#define QML_BEGIN_INSTR_COMMON(I) { \
    const QQmlInstructionMeta<(int)QQmlInstruction::I>::DataType &instr = QQmlInstructionMeta<(int)QQmlInstruction::I>::data(*genericInstr); \
    INSTRUCTIONSTREAM += QQmlInstructionMeta<(int)QQmlInstruction::I>::Size; \
    Q_UNUSED(instr);

#ifdef QML_THREADED_VME_INTERPRETER
#  define QML_BEGIN_INSTR(I) op_##I: \
    QML_BEGIN_INSTR_COMMON(I)

#  define QML_NEXT_INSTR(I) { \
    if (watcher.hasRecursed()) return 0; \
    genericInstr = reinterpret_cast<const QQmlInstruction *>(INSTRUCTIONSTREAM); \
    goto *genericInstr->common.code; \
    }

#  define QML_END_INSTR(I) } \
    if (watcher.hasRecursed()) return 0; \
    genericInstr = reinterpret_cast<const QQmlInstruction *>(INSTRUCTIONSTREAM); \
    if (interrupt.shouldInterrupt()) return 0; \
    goto *genericInstr->common.code;

#else
#  define QML_BEGIN_INSTR(I) \
    case QQmlInstruction::I: \
    QML_BEGIN_INSTR_COMMON(I)

#  define QML_NEXT_INSTR(I) { \
    if (watcher.hasRecursed()) return 0; \
    break; \
    }

#  define QML_END_INSTR(I) \
    if (watcher.hasRecursed() || interrupt.shouldInterrupt()) return 0; \
    } break;
#endif

#define QML_STORE_VALUE(name, cpptype, value) \
    QML_BEGIN_INSTR(name) \
        cpptype v = value; \
        void *a[] = { (void *)&v, 0, &status, &flags }; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QMetaObject::metacall(target, QMetaObject::WriteProperty, instr.propertyIndex, a); \
    QML_END_INSTR(name)

#define QML_STORE_PROVIDER_VALUE(name, type, value) \
    QML_BEGIN_INSTR(name) \
        struct { void *data[4]; } buffer; \
        if (QQml_valueTypeProvider()->storeValueType(type, &value, &buffer, sizeof(buffer))) { \
            void *a[] = { reinterpret_cast<void *>(&buffer), 0, &status, &flags }; \
            QObject *target = objects.top(); \
            CLEAN_PROPERTY(target, instr.propertyIndex); \
            QMetaObject::metacall(target, QMetaObject::WriteProperty, instr.propertyIndex, a); \
        } \
    QML_END_INSTR(name)

#define QML_STORE_LIST(name, cpptype, value) \
    QML_BEGIN_INSTR(name) \
        cpptype v; \
        v.append(value); \
        void *a[] = { (void *)&v, 0, &status, &flags }; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QMetaObject::metacall(target, QMetaObject::WriteProperty, instr.propertyIndex, a); \
    QML_END_INSTR(name)

#define QML_STORE_VAR(name, value) \
    QML_BEGIN_INSTR(name) \
        v8::Handle<v8::Value> v8value = value; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(target); \
        Q_ASSERT(vmemo); \
        vmemo->setVMEProperty(instr.propertyIndex, v8value); \
    QML_END_INSTR(name)

#define QML_STORE_POINTER(name, value) \
    QML_BEGIN_INSTR(name) \
        void *a[] = { (void *)value, 0, &status, &flags }; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QMetaObject::metacall(target, QMetaObject::WriteProperty, instr.propertyIndex, a); \
    QML_END_INSTR(name)

#define CLEAN_PROPERTY(o, index) \
    if (fastHasBinding(o, index)) \
        removeBindingOnProperty(o, index)

QObject *QQmlVME::run(QList<QQmlError> *errors,
                              const Interrupt &interrupt
#ifdef QML_THREADED_VME_INTERPRETER
                              , void * const **storeJumpTable
#endif
                              )
{
#ifdef QML_THREADED_VME_INTERPRETER
    if (storeJumpTable) {
#define QML_INSTR_ADDR(I, FMT) &&op_##I,
        static void *const jumpTable[] = {
            FOR_EACH_QML_INSTR(QML_INSTR_ADDR)
        };
#undef QML_INSTR_ADDR
        *storeJumpTable = jumpTable;
        return 0;
    }
#endif
    Q_ASSERT(errors->isEmpty());
    Q_ASSERT(states.count() >= 1);

    QQmlEngine *engine = states.at(0).context->engine;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    // Need a v8 handle scope and execution context for StoreVar instructions.
    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(ep->v8engine()->context());

    int status = -1; // needed for dbus
    QQmlPropertyPrivate::WriteFlags flags = QQmlPropertyPrivate::BypassInterceptor |
                                                    QQmlPropertyPrivate::RemoveBindingOnAliasWrite;

    QRecursionWatcher<QQmlVME, &QQmlVME::recursion> watcher(this);

#define COMP states.top().compiledData
#define CTXT states.top().context
#define INSTRUCTIONSTREAM states.top().instructionStream
#define BINDINGSKIPLIST states.top().bindingSkipList

#define TYPES COMP->types
#define PRIMITIVES COMP->primitives
#define DATAS COMP->datas
#define PROGRAMS COMP->programs
#define PROPERTYCACHES COMP->propertyCaches
#define SCRIPTS COMP->scripts
#define URLS COMP->urls

#ifdef QML_THREADED_VME_INTERPRETER
    const QQmlInstruction *genericInstr = reinterpret_cast<const QQmlInstruction *>(INSTRUCTIONSTREAM);
    goto *genericInstr->common.code;
#else
    for (;;) {
        const QQmlInstruction *genericInstr = reinterpret_cast<const QQmlInstruction *>(INSTRUCTIONSTREAM);

        switch (genericInstr->common.instructionType) {
#endif

        // Store a created object in a property.  These all pop from the objects stack.
        QML_STORE_VALUE(StoreObject, QObject *, objects.pop());
        QML_STORE_VALUE(StoreVariantObject, QVariant, QVariant::fromValue(objects.pop()));
        QML_STORE_VAR(StoreVarObject, ep->v8engine()->newQObject(objects.pop()));

        // Store a literal value in a corresponding property
        QML_STORE_VALUE(StoreFloat, float, instr.value);
        QML_STORE_VALUE(StoreDouble, double, instr.value);
        QML_STORE_VALUE(StoreBool, bool, instr.value);
        QML_STORE_VALUE(StoreInteger, int, instr.value);
        QML_STORE_PROVIDER_VALUE(StoreColor, QMetaType::QColor, instr.value);
        QML_STORE_VALUE(StoreDate, QDate, QDate::fromJulianDay(instr.value));
        QML_STORE_VALUE(StoreDateTime, QDateTime,
                        QDateTime(QDate::fromJulianDay(instr.date), *(QTime *)&instr.time));
        QML_STORE_POINTER(StoreTime, (QTime *)&instr.time);
        QML_STORE_POINTER(StorePoint, (QPoint *)&instr.point);
        QML_STORE_POINTER(StorePointF, (QPointF *)&instr.point);
        QML_STORE_POINTER(StoreSize, (QSize *)&instr.size);
        QML_STORE_POINTER(StoreSizeF, (QSizeF *)&instr.size);
        QML_STORE_POINTER(StoreRect, (QRect *)&instr.rect);
        QML_STORE_POINTER(StoreRectF, (QRectF *)&instr.rect);
        QML_STORE_PROVIDER_VALUE(StoreVector3D, QMetaType::QVector3D, instr.vector);
        QML_STORE_PROVIDER_VALUE(StoreVector4D, QMetaType::QVector4D, instr.vector);
        QML_STORE_POINTER(StoreString, &PRIMITIVES.at(instr.value));
        QML_STORE_POINTER(StoreByteArray, &DATAS.at(instr.value));
        QML_STORE_POINTER(StoreUrl, &URLS.at(instr.value));
#ifndef QT_NO_TRANSLATION
        QML_STORE_VALUE(StoreTrString, QString,
                        QCoreApplication::translate(DATAS.at(instr.context).constData(),
                                                    DATAS.at(instr.text).constData(),
                                                    DATAS.at(instr.comment).constData(),
                                                    instr.n));
        QML_STORE_VALUE(StoreTrIdString, QString, qtTrId(DATAS.at(instr.text).constData(), instr.n));
#endif

        // Store a literal value in a QList
        QML_STORE_LIST(StoreStringList, QStringList, PRIMITIVES.at(instr.value));
        QML_STORE_LIST(StoreStringQList, QList<QString>, PRIMITIVES.at(instr.value));
        QML_STORE_LIST(StoreUrlQList, QList<QUrl>, URLS.at(instr.value));
        QML_STORE_LIST(StoreDoubleQList, QList<double>, instr.value);
        QML_STORE_LIST(StoreBoolQList, QList<bool>, instr.value);
        QML_STORE_LIST(StoreIntegerQList, QList<int>, instr.value);

        // Store a literal value in a QVariant property
        QML_STORE_VALUE(StoreVariant, QVariant, variantFromString(PRIMITIVES.at(instr.value)));
        QML_STORE_VALUE(StoreVariantInteger, QVariant, QVariant(instr.value));
        QML_STORE_VALUE(StoreVariantDouble, QVariant, QVariant(instr.value));
        QML_STORE_VALUE(StoreVariantBool, QVariant, QVariant(instr.value));

        // Store a literal value in a var property.
        // We deliberately do not use string converters here
        QML_STORE_VAR(StoreVar, ep->v8engine()->fromVariant(PRIMITIVES.at(instr.value)));
        QML_STORE_VAR(StoreVarInteger, v8::Integer::New(instr.value));
        QML_STORE_VAR(StoreVarDouble, v8::Number::New(instr.value));
        QML_STORE_VAR(StoreVarBool, v8::Boolean::New(instr.value));

        // Store a literal value in a QJSValue property.
        QML_STORE_VALUE(StoreJSValueString, QJSValue, QJSValue(PRIMITIVES.at(instr.value)));
        QML_STORE_VALUE(StoreJSValueInteger, QJSValue, QJSValue(instr.value));
        QML_STORE_VALUE(StoreJSValueDouble, QJSValue, QJSValue(instr.value));
        QML_STORE_VALUE(StoreJSValueBool, QJSValue, QJSValue(instr.value));

        QML_BEGIN_INSTR(Init)
            // Ensure that the compiled data has been initialized
            if (!COMP->isInitialized()) COMP->initialize(engine);

            QQmlContextData *parentCtxt = CTXT;
            CTXT = new QQmlContextData;
            CTXT->isInternal = true;
            CTXT->url = COMP->url;
            CTXT->urlString = COMP->name;
            CTXT->imports = COMP->importCache;
            CTXT->imports->addref();
            CTXT->setParent(parentCtxt);
            if (instr.contextCache != -1) 
                CTXT->setIdPropertyData(COMP->contextCaches.at(instr.contextCache));
            if (instr.compiledBinding != -1) {
                const char *v4data = DATAS.at(instr.compiledBinding).constData();
                CTXT->v4bindings = new QV4Bindings(v4data, CTXT);
            }
            if (states.count() == 1) {
                rootContext = CTXT;
                rootContext->activeVMEData = data;
                rootContext->isRootObjectInCreation = true;
            }
            if (states.count() == 1 && !creationContext.isNull()) {
                // A component that is logically created within another component instance shares the 
                // same instances of script imports.  For example:
                //
                //     import QtQuick 2.0
                //     import "test.js" as Test
                //     ListView {
                //         model: Test.getModel()
                //         delegate: Component {
                //             Text { text: Test.getValue(index); }
                //         }
                //     }
                //
                // Has the same "Test" instance.  To implement this, we simply copy the v8 handles into
                // the inner context.  We have to create a fresh persistent handle for each to prevent 
                // double dispose.  It is possible we could do this more efficiently using some form of
                // referencing instead.
                CTXT->importedScripts = creationContext->importedScripts;
                for (int ii = 0; ii < CTXT->importedScripts.count(); ++ii)
                    CTXT->importedScripts[ii] = qPersistentNew<v8::Object>(CTXT->importedScripts[ii]);
            }
        QML_END_INSTR(Init)

        QML_BEGIN_INSTR(DeferInit)
        QML_END_INSTR(DeferInit)

        QML_BEGIN_INSTR(Done)
            states.pop();

            if (states.isEmpty())
                goto normalExit;
        QML_END_INSTR(Done)

        QML_BEGIN_INSTR(CreateQMLObject)
            const QQmlCompiledData::TypeReference &type = TYPES.at(instr.type);
            Q_ASSERT(type.component);

            states.push(State());

            State *cState = &states[states.count() - 2];
            State *nState = &states[states.count() - 1];

            nState->context = cState->context;
            nState->compiledData = type.component;
            nState->instructionStream = type.component->bytecode.constData();

            if (instr.bindingBits != -1) {
                const QByteArray &bits = cState->compiledData->datas.at(instr.bindingBits);
                nState->bindingSkipList = QBitField((const quint32*)bits.constData(),
                                                    bits.size() * 8);
            }
            if (instr.isRoot)
                nState->bindingSkipList = nState->bindingSkipList.united(cState->bindingSkipList);

            // As the state in the state stack changed, execution will continue in the new program.
        QML_END_INSTR(CreateQMLObject)

        QML_BEGIN_INSTR(CompleteQMLObject)
            QObject *o = objects.top();
            Q_ASSERT(o);

            QQmlData *ddata = QQmlData::get(o);
            Q_ASSERT(ddata);

            if (states.count() == 1) {
                // Keep a reference to the compiled data we rely on.
                // Only the top-level component instance needs to add a reference - higher-level
                // components add a reference to the components they depend on, so an instance
                // of the top-level component keeps them all referenced.
                ddata->compiledData = states[0].compiledData;
                ddata->compiledData->addref();
            }

            if (instr.isRoot) {
                if (ddata->context) {
                    Q_ASSERT(ddata->context != CTXT);
                    Q_ASSERT(ddata->outerContext);
                    Q_ASSERT(ddata->outerContext != CTXT);
                    QQmlContextData *c = ddata->context;
                    while (c->linkedContext) c = c->linkedContext;
                    c->linkedContext = CTXT;
                } else {
                    CTXT->addObject(o);
                }

                ddata->ownContext = true;
            } else if (!ddata->context) {
                CTXT->addObject(o);
            }

            ddata->setImplicitDestructible();
            ddata->outerContext = CTXT;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;
        QML_END_INSTR(CompleteQMLObject)

        QML_BEGIN_INSTR(CreateCppObject)
            const QQmlCompiledData::TypeReference &type = TYPES.at(instr.type);
            Q_ASSERT(type.type);

            QObject *o = 0;
            void *memory = 0;
            type.type->create(&o, &memory, sizeof(QQmlData));

            if (!o)
                VME_EXCEPTION(tr("Unable to create object of type %1").arg(type.type->elementName()),
                              instr.line);

            QQmlData *ddata = new (memory) QQmlData;
            ddata->ownMemory = false;
            QObjectPrivate::get(o)->declarativeData = ddata;

            if (rootContext && rootContext->isRootObjectInCreation) {
                ddata->rootObjectInCreation = true;
                rootContext->isRootObjectInCreation = false;
            }

            if (type.typePropertyCache && !ddata->propertyCache) {
                ddata->propertyCache = type.typePropertyCache;
                ddata->propertyCache->addref();
            }

            if (states.count() == 1) {
                // Keep a reference to the compiled data we rely on
                ddata->compiledData = states[0].compiledData;
                ddata->compiledData->addref();
            }

            if (instr.isRoot) {
                if (ddata->context) {
                    Q_ASSERT(ddata->context != CTXT);
                    Q_ASSERT(ddata->outerContext);
                    Q_ASSERT(ddata->outerContext != CTXT);
                    QQmlContextData *c = ddata->context;
                    while (c->linkedContext) c = c->linkedContext;
                    c->linkedContext = CTXT;
                } else {
                    CTXT->addObject(o);
                }

                ddata->ownContext = true;
            } else if (!ddata->context) {
                CTXT->addObject(o);
            }

            ddata->setImplicitDestructible();
            ddata->outerContext = CTXT;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            if (instr.data != -1) {
                QQmlCustomParser *customParser =
                    TYPES.at(instr.type).type->customParser();
                customParser->setCustomData(o, DATAS.at(instr.data));
            }
            if (!objects.isEmpty()) {
                QObject *parent = objects.at(objects.count() - 1 - (instr.parentToSuper?1:0));
#if 0 // ### refactor
                if (o->isWidgetType() && parent->isWidgetType()) 
                    static_cast<QWidget*>(o)->setParent(static_cast<QWidget*>(parent));
                else 
#endif
                    QQml_setParent_noEvent(o, parent);
                ddata->parentFrozen = true;
            }
            objects.push(o);
        QML_END_INSTR(CreateCppObject)

        QML_BEGIN_INSTR(CreateSimpleObject)
            QObject *o = (QObject *)operator new(instr.typeSize + sizeof(QQmlData));   
            ::memset(o, 0, instr.typeSize + sizeof(QQmlData));
            instr.create(o);

            QQmlData *ddata = (QQmlData *)(((const char *)o) + instr.typeSize);
            const QQmlCompiledData::TypeReference &ref = TYPES.at(instr.type);
            if (!ddata->propertyCache && ref.typePropertyCache) {
                ddata->propertyCache = ref.typePropertyCache;
                ddata->propertyCache->addref();
            }
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            QObjectPrivate::get(o)->declarativeData = ddata;                                                      
            ddata->context = ddata->outerContext = CTXT;
            ddata->nextContextObject = CTXT->contextObjects; 
            if (ddata->nextContextObject) 
                ddata->nextContextObject->prevContextObject = &ddata->nextContextObject; 
            ddata->prevContextObject = &CTXT->contextObjects; 
            CTXT->contextObjects = ddata; 

            QObject *parent = objects.at(objects.count() - 1 - (instr.parentToSuper?1:0));
            QQml_setParent_noEvent(o, parent);                                                        

            ddata->parentFrozen = true;
            objects.push(o);
        QML_END_INSTR(CreateSimpleObject)

        QML_BEGIN_INSTR(SetId)
            QObject *target = objects.top();
            CTXT->setIdProperty(instr.index, target);
        QML_END_INSTR(SetId)

        QML_BEGIN_INSTR(SetDefault)
            CTXT->contextObject = objects.top();
        QML_END_INSTR(SetDefault)

        QML_BEGIN_INSTR(CreateComponent)
            QQmlComponent *qcomp = 
                new QQmlComponent(CTXT->engine, COMP, INSTRUCTIONSTREAM - COMP->bytecode.constData(),
                                          objects.isEmpty() ? 0 : objects.top());

            QQmlData *ddata = QQmlData::get(qcomp, true);
            Q_ASSERT(ddata);

            CTXT->addObject(qcomp);

            if (states.count() == 1) {
                // Keep a reference to the compiled data we rely on
                ddata->compiledData = states[0].compiledData;
                ddata->compiledData->addref();
            }

            if (instr.isRoot)
                ddata->ownContext = true;

            ddata->setImplicitDestructible();
            ddata->outerContext = CTXT;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            QQmlComponentPrivate::get(qcomp)->creationContext = CTXT;

            objects.push(qcomp);
            INSTRUCTIONSTREAM += instr.count;
        QML_END_INSTR(CreateComponent)

        QML_BEGIN_INSTR(StoreMetaObject)
            QObject *target = objects.top();

            QQmlPropertyCache *propertyCache = PROPERTYCACHES.at(instr.propertyCache);

            const QQmlVMEMetaData *data = 
                (const QQmlVMEMetaData *)DATAS.at(instr.aliasData).constData();

            (void)new QQmlVMEMetaObject(target, propertyCache, data);

            QQmlData *ddata = QQmlData::get(target, true);
            if (ddata->propertyCache) ddata->propertyCache->release();
            ddata->propertyCache = propertyCache;
            ddata->propertyCache->addref();

        QML_END_INSTR(StoreMetaObject)

        QML_BEGIN_INSTR(AssignCustomType)
            QObject *target = objects.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            const QString &primitive = PRIMITIVES.at(instr.primitive);
            int type = instr.type;
            QQmlMetaType::StringConverter converter = QQmlMetaType::customStringConverter(type);
            QVariant v = (*converter)(primitive);

            QMetaProperty prop = 
                    target->metaObject()->property(instr.propertyIndex);
            if (v.isNull() || ((int)prop.type() != type && prop.userType() != type)) 
                VME_EXCEPTION(tr("Cannot assign value %1 to property %2").arg(primitive).arg(QString::fromUtf8(prop.name())), instr.line);

            void *a[] = { (void *)v.data(), 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(AssignCustomType)

        QML_BEGIN_INSTR(AssignSignalObject)
            // XXX optimize

            QObject *assign = objects.pop();
            QObject *target = objects.top();
            int sigIdx = instr.signal;
            const QString &pr = PRIMITIVES.at(sigIdx);

            QQmlProperty prop(target, pr);
            if (prop.type() & QQmlProperty::SignalProperty) {

                QMetaMethod method = QQmlMetaType::defaultMethod(assign);
                if (!method.isValid())
                    VME_EXCEPTION(tr("Cannot assign object type %1 with no default method").arg(QString::fromLatin1(assign->metaObject()->className())), instr.line);

                if (!QMetaObject::checkConnectArgs(prop.method(), method)) {
                    VME_EXCEPTION(tr("Cannot connect mismatched signal/slot %1 %vs. %2")
                                  .arg(QString::fromLatin1(method.methodSignature().constData()))
                                  .arg(QString::fromLatin1(prop.method().methodSignature().constData())), instr.line);
                }

                QQmlPropertyPrivate::connect(target, prop.index(), assign, method.methodIndex());

            } else {
                VME_EXCEPTION(tr("Cannot assign an object to signal property %1").arg(pr), instr.line);
            }


        QML_END_INSTR(AssignSignalObject)

        QML_BEGIN_INSTR(StoreSignal)
            QObject *target = objects.top();
            QObject *context = objects.at(objects.count() - 1 - instr.context);

            QQmlBoundSignal *bs = new QQmlBoundSignal(target, instr.signalIndex, target, engine);
            QQmlBoundSignalExpression *expr =
                new QQmlBoundSignalExpression(target, instr.signalIndex,
                                              CTXT, context, DATAS.at(instr.value),
                                              true, COMP->name, instr.line, instr.column);
            expr->setParameterCountForJS(instr.parameterCount);
            bs->takeExpression(expr);
        QML_END_INSTR(StoreSignal)

        QML_BEGIN_INSTR(StoreImportedScript)
            CTXT->importedScripts << run(CTXT, SCRIPTS.at(instr.value));
        QML_END_INSTR(StoreImportedScript)

        QML_BEGIN_INSTR(StoreScriptString)
            QObject *target = objects.top();
            QObject *scope = objects.at(objects.count() - 1 - instr.scope);
            QQmlScriptString ss(PRIMITIVES.at(instr.value), CTXT->asQQmlContext(), scope);
            ss.d.data()->bindingId = instr.bindingId;
            ss.d.data()->lineNumber = qmlSourceCoordinate(instr.line);
            ss.d.data()->columnNumber = qmlSourceCoordinate(instr.column);
            ss.d.data()->isStringLiteral = instr.isStringLiteral;
            ss.d.data()->isNumberLiteral = instr.isNumberLiteral;
            ss.d.data()->numberValue = instr.numberValue;

            void *a[] = { &ss, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreScriptString)

        QML_BEGIN_INSTR(BeginObject)
            QObject *target = objects.top();
            QQmlParserStatus *status = reinterpret_cast<QQmlParserStatus *>(reinterpret_cast<char *>(target) + instr.castValue);
            parserStatus.push(status);
#ifdef QML_ENABLE_TRACE
            Q_ASSERT(QObjectPrivate::get(target)->declarativeData);
            parserStatusData.push(static_cast<QQmlData *>(QObjectPrivate::get(target)->declarativeData));
#endif
            status->d = &parserStatus.top();

            status->classBegin();
        QML_END_INSTR(BeginObject)

        QML_BEGIN_INSTR(InitV8Bindings)
            CTXT->v8bindings = new QV8Bindings(&PROGRAMS[instr.programIndex], instr.line, CTXT);
        QML_END_INSTR(InitV8Bindings)

        QML_BEGIN_INSTR(StoreBinding)
            QObject *target = 
                objects.at(objects.count() - 1 - instr.owner);
            QObject *context = 
                objects.at(objects.count() - 1 - instr.context);

            if (instr.isRoot && BINDINGSKIPLIST.testBit(instr.property.coreIndex))
                QML_NEXT_INSTR(StoreBinding);

            QQmlBinding *bind = new QQmlBinding(PRIMITIVES.at(instr.value), true, 
                                                context, CTXT, COMP->name, instr.line,
                                                instr.column);
            bindValues.push(bind);
            bind->m_mePtr = &bindValues.top();
            bind->setTarget(target, instr.property, CTXT);

            if (instr.isAlias) {
                QQmlAbstractBinding *old =
                    QQmlPropertyPrivate::setBindingNoEnable(target,
                                                            instr.property.coreIndex,
                                                            instr.property.getValueTypeCoreIndex(),
                                                            bind);
                if (old) { old->destroy(); }
            } else {
                typedef QQmlPropertyPrivate QDPP;
                Q_ASSERT(bind->propertyIndex() == QDPP::bindingIndex(instr.property));
                Q_ASSERT(bind->object() == target);

                CLEAN_PROPERTY(target, QDPP::bindingIndex(instr.property));

                bind->addToObject();
            }
        QML_END_INSTR(StoreBinding)

        QML_BEGIN_INSTR(StoreV4Binding)
            QObject *target = 
                objects.at(objects.count() - 1 - instr.owner);
            QObject *scope = 
                objects.at(objects.count() - 1 - instr.context);

            int propertyIdx = (instr.property & 0x0000FFFF);

            if (instr.isRoot && BINDINGSKIPLIST.testBit(propertyIdx))
                QML_NEXT_INSTR(StoreV4Binding);

            QQmlAbstractBinding *binding = CTXT->v4bindings->configBinding(target, scope, &instr);
            bindValues.push(binding);
            binding->m_mePtr = &bindValues.top();

            if (instr.isAlias) {
                QQmlAbstractBinding *old =
                    QQmlPropertyPrivate::setBindingNoEnable(target,
                                                            propertyIdx,
                                                            instr.propType ? (instr.property >> 16) : -1,
                                                            binding);
                if (old) { old->destroy(); }
            } else {
                Q_ASSERT(binding->propertyIndex() == instr.property);
                Q_ASSERT(binding->object() == target);

                CLEAN_PROPERTY(target, instr.property);

                binding->addToObject();

                if (instr.propType == 0) {
                    // All non-valuetype V4 bindings are safe bindings
                    QQmlData *data = QQmlData::get(target);
                    Q_ASSERT(data);
                    data->setPendingBindingBit(target, propertyIdx);
                }
            }
        QML_END_INSTR(StoreV4Binding)

        QML_BEGIN_INSTR(StoreV8Binding)
            QObject *target = 
                objects.at(objects.count() - 1 - instr.owner);
            QObject *scope = 
                objects.at(objects.count() - 1 - instr.context);

            int coreIndex = instr.property.coreIndex;

            if (instr.isRoot && BINDINGSKIPLIST.testBit(coreIndex))
                QML_NEXT_INSTR(StoreV8Binding);

            QQmlAbstractBinding *binding = CTXT->v8bindings->configBinding(target, scope,
                                                                                   &instr);
            if (binding && !instr.isFallback) {
                bindValues.push(binding);
                binding->m_mePtr = &bindValues.top();

                if (instr.isAlias) {
                    QQmlAbstractBinding *old =
                        QQmlPropertyPrivate::setBindingNoEnable(target, coreIndex,
                                                                instr.property.getValueTypeCoreIndex(),
                                                                binding);
                    if (old) { old->destroy(); }
                } else {
                    typedef QQmlPropertyPrivate QDPP;
                    Q_ASSERT(binding->propertyIndex() == QDPP::bindingIndex(instr.property));
                    Q_ASSERT(binding->object() == target);

                    CLEAN_PROPERTY(target, QDPP::bindingIndex(instr.property));

                    binding->addToObject();

                    if (instr.isSafe && !instr.property.isValueTypeVirtual()) {
                        QQmlData *data = QQmlData::get(target);
                        Q_ASSERT(data);
                        data->setPendingBindingBit(target, coreIndex);
                    }
                }
            }
        QML_END_INSTR(StoreV8Binding)

        QML_BEGIN_INSTR(StoreValueSource)
            QObject *obj = objects.pop();
            QQmlPropertyValueSource *vs = reinterpret_cast<QQmlPropertyValueSource *>(reinterpret_cast<char *>(obj) + instr.castValue);
            QObject *target = obj->parent();
            vs->setTarget(QQmlPropertyPrivate::restore(target, instr.property, CTXT));
        QML_END_INSTR(StoreValueSource)

        QML_BEGIN_INSTR(StoreValueInterceptor)
            QObject *obj = objects.pop();
            QQmlPropertyValueInterceptor *vi = reinterpret_cast<QQmlPropertyValueInterceptor *>(reinterpret_cast<char *>(obj) + instr.castValue);
            QObject *target = obj->parent();
            QQmlProperty prop = 
                QQmlPropertyPrivate::restore(target, instr.property, CTXT);
            vi->setTarget(prop);
            QQmlVMEMetaObject *mo = QQmlVMEMetaObject::get(target);
            Q_ASSERT(mo);
            mo->registerInterceptor(prop.index(), QQmlPropertyPrivate::valueTypeCoreIndex(prop), vi);
        QML_END_INSTR(StoreValueInterceptor)

        QML_BEGIN_INSTR(StoreObjectQList)
            QObject *assign = objects.pop();

            const List &list = lists.top();
            if (list.qListProperty.append)
                list.qListProperty.append((QQmlListProperty<void>*)&list.qListProperty, assign);
            else
                VME_EXCEPTION(tr("Cannot assign object to read only list"), -1);
        QML_END_INSTR(StoreObjectQList)

        QML_BEGIN_INSTR(AssignObjectList)
            // This is only used for assigning interfaces
            QObject *assign = objects.pop();
            const List &list = lists.top();

            int type = list.type;

            void *ptr = 0;

            const char *iid = QQmlMetaType::interfaceIId(type);
            if (iid) 
                ptr = assign->qt_metacast(iid);
            if (!ptr) 
                VME_EXCEPTION(tr("Cannot assign object to list"), instr.line);

            if (list.qListProperty.append)
                list.qListProperty.append((QQmlListProperty<void>*)&list.qListProperty, ptr);
            else
                VME_EXCEPTION(tr("Cannot assign object to read only list"), -1);
        QML_END_INSTR(AssignObjectList)

        QML_BEGIN_INSTR(StoreInterface)
            QObject *assign = objects.pop();
            QObject *target = objects.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            int coreIdx = instr.propertyIndex;
            QMetaProperty prop = target->metaObject()->property(coreIdx);
            int t = prop.userType();
            const char *iid = QQmlMetaType::interfaceIId(t);
            bool ok = false;
            if (iid) {
                void *ptr = assign->qt_metacast(iid);
                if (ptr) {
                    void *a[] = { &ptr, 0, &status, &flags };
                    QMetaObject::metacall(target, 
                                          QMetaObject::WriteProperty,
                                          coreIdx, a);
                    ok = true;
                }
            } 

            if (!ok) 
                VME_EXCEPTION(tr("Cannot assign object to interface property"), instr.line);
        QML_END_INSTR(StoreInterface)
            
        QML_BEGIN_INSTR(FetchAttached)
            QObject *target = objects.top();

            QObject *qmlObject = qmlAttachedPropertiesObjectById(instr.id, target);

            if (!qmlObject)
                VME_EXCEPTION(tr("Unable to create attached object"), instr.line);

            objects.push(qmlObject);
        QML_END_INSTR(FetchAttached)

        QML_BEGIN_INSTR(FetchQList)
            QObject *target = objects.top();

            lists.push(List(instr.type));

            void *a[1];
            a[0] = (void *)&(lists.top().qListProperty);
            QMetaObject::metacall(target, QMetaObject::ReadProperty, 
                                  instr.property, a);
        QML_END_INSTR(FetchQList)

        QML_BEGIN_INSTR(FetchObject)
            QObject *target = objects.top();

            QObject *obj = 0;
            // NOTE: This assumes a cast to QObject does not alter the 
            // object pointer
            void *a[1];
            a[0] = &obj;
            QMetaObject::metacall(target, QMetaObject::ReadProperty, 
                                  instr.property, a);

            if (!obj)
                VME_EXCEPTION(tr("Cannot set properties on %1 as it is null").arg(QString::fromUtf8(target->metaObject()->property(instr.property).name())), instr.line);

            objects.push(obj);
        QML_END_INSTR(FetchObject)

        QML_BEGIN_INSTR(PopQList)
            lists.pop();
        QML_END_INSTR(PopQList)

        QML_BEGIN_INSTR(Defer)
            if (instr.deferCount) {
                QObject *target = objects.top();
                QQmlData *data = QQmlData::get(target, true);
                if (data->deferredData) {
                    //This rare case still won't always work right
                    qmlInfo(target) << "Setting deferred property across multiple components may not work";
                    delete data->deferredData;
                }
                data->deferredData = new QQmlData::DeferredData;
                //If we're in a CreateQML here, data->compiledData could be reset later
                data->deferredData->compiledData = COMP;
                data->deferredData->context = CTXT;
                // Keep this data referenced until we're initialized
                data->deferredData->compiledData->addref();
                data->deferredData->deferredIdx = INSTRUCTIONSTREAM - COMP->bytecode.constData();
                Q_ASSERT(data->deferredData->deferredIdx != 0);
                INSTRUCTIONSTREAM += instr.deferCount;
            }
        QML_END_INSTR(Defer)

        QML_BEGIN_INSTR(PopFetchedObject)
            objects.pop();
        QML_END_INSTR(PopFetchedObject)

        QML_BEGIN_INSTR(FetchValueType)
            QObject *target = objects.top();

            if (instr.bindingSkipList != 0) {
                // Possibly need to clear bindings
                QQmlData *targetData = QQmlData::get(target);
                if (targetData) {
                    QQmlAbstractBinding *binding = 
                        QQmlPropertyPrivate::binding(target, instr.property, -1);

                    if (binding && binding->bindingType() != QQmlAbstractBinding::ValueTypeProxy) {
                        QQmlPropertyPrivate::setBinding(target, instr.property, -1, 0);
                        binding->destroy();
                    } else if (binding) {
                        QQmlValueTypeProxyBinding *proxy = 
                            static_cast<QQmlValueTypeProxyBinding *>(binding);
                        proxy->removeBindings(instr.bindingSkipList);
                    }
                }
            }

            QQmlValueType *valueHandler = QQmlValueTypeFactory::valueType(instr.type);
            Q_ASSERT(valueHandler);
            valueHandler->read(target, instr.property);
            objects.push(valueHandler);
        QML_END_INSTR(FetchValueType)

        QML_BEGIN_INSTR(PopValueType)
            QQmlValueType *valueHandler = 
                static_cast<QQmlValueType *>(objects.pop());
            QObject *target = objects.top();
            valueHandler->write(target, instr.property, QQmlPropertyPrivate::BypassInterceptor);
        QML_END_INSTR(PopValueType)

#ifdef QML_THREADED_VME_INTERPRETER
    // nothing to do
#else
        default:
            Q_UNREACHABLE();
            qFatal("QQmlCompiledData: Internal error - unknown instruction %d", genericInstr->common.instructionType);
            break;
        }
    }
#endif

exceptionExit:
    Q_ASSERT(!states.isEmpty());
    Q_ASSERT(!errors->isEmpty());

    reset();

    return 0;

normalExit:
    Q_ASSERT(objects.count() == 1);

    QObject *rv = objects.top();

    objects.deallocate();
    lists.deallocate();
    states.clear();

    return rv;
}

void QQmlVME::reset()
{
    Q_ASSERT(!states.isEmpty() || objects.isEmpty());

    QRecursionWatcher<QQmlVME, &QQmlVME::recursion> watcher(this);

    if (!objects.isEmpty() && !(states.at(0).flags & State::Deferred))
        delete objects.at(0); 
    
    if (!rootContext.isNull()) 
        rootContext->activeVMEData = 0;

    // Remove the QQmlParserStatus and QQmlAbstractBinding back pointers
    blank(parserStatus);
    blank(bindValues);

    while (componentAttached) {
        QQmlComponentAttached *a = componentAttached;
        a->rem();
    }
    
    engine = 0;
    objects.deallocate();
    lists.deallocate();
    bindValues.deallocate();
    parserStatus.deallocate();
#ifdef QML_ENABLE_TRACE
    parserStatusData.deallocate();
#endif
    finalizeCallbacks.clear();
    states.clear();
    rootContext = 0;
    creationContext = 0;
}

// Must be called with a handle scope and context
void QQmlScriptData::initialize(QQmlEngine *engine)
{
    Q_ASSERT(m_program.IsEmpty());
    Q_ASSERT(engine);
    Q_ASSERT(!hasEngine());

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
    QV8Engine *v8engine = ep->v8engine();

    // If compilation throws an error, a surrounding v8::TryCatch will record it.
    v8::Local<v8::Script> program = v8engine->qmlModeCompile(m_programSource.constData(),
                                                             m_programSource.length(), urlString, 1);
    if (program.IsEmpty())
        return;

    m_program = qPersistentNew<v8::Script>(program);
    m_programSource.clear(); // We don't need this anymore

    addToEngine(engine);

    addref();
}

v8::Persistent<v8::Object> QQmlVME::run(QQmlContextData *parentCtxt, QQmlScriptData *script)
{
    if (script->m_loaded)
        return qPersistentNew<v8::Object>(script->m_value);

    v8::Persistent<v8::Object> rv;

    Q_ASSERT(parentCtxt && parentCtxt->engine);
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(parentCtxt->engine);
    QV8Engine *v8engine = ep->v8engine();

    if (script->hasError()) {
        ep->warning(script->error());
        return rv;
    }

    bool shared = script->pragmas & QQmlScript::Object::ScriptBlock::Shared;

    QQmlContextData *effectiveCtxt = parentCtxt;
    if (shared)
        effectiveCtxt = 0;

    // Create the script context if required
    QQmlContextData *ctxt = new QQmlContextData;
    ctxt->isInternal = true;
    ctxt->isJSContext = true;
    if (shared)
        ctxt->isPragmaLibraryContext = true;
    else
        ctxt->isPragmaLibraryContext = parentCtxt->isPragmaLibraryContext;
    ctxt->url = script->url;
    ctxt->urlString = script->urlString;

    // For backward compatibility, if there are no imports, we need to use the
    // imports from the parent context.  See QTBUG-17518.
    if (!script->importCache->isEmpty()) {
        ctxt->imports = script->importCache;
    } else if (effectiveCtxt) {
        ctxt->imports = effectiveCtxt->imports;
        ctxt->importedScripts = effectiveCtxt->importedScripts;
        for (int ii = 0; ii < ctxt->importedScripts.count(); ++ii)
            ctxt->importedScripts[ii] = qPersistentNew<v8::Object>(ctxt->importedScripts[ii]);
    }

    if (ctxt->imports) {
        ctxt->imports->addref();
    }

    if (effectiveCtxt) {
        ctxt->setParent(effectiveCtxt, true);
    } else {
        ctxt->engine = parentCtxt->engine; // Fix for QTBUG-21620
    }

    for (int ii = 0; ii < script->scripts.count(); ++ii) {
        ctxt->importedScripts << run(ctxt, script->scripts.at(ii)->scriptData());
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(v8engine->context());

    v8::TryCatch try_catch;
    if (!script->isInitialized())
        script->initialize(parentCtxt->engine);

    v8::Local<v8::Object> qmlglobal = v8engine->qmlScope(ctxt, 0);
    v8engine->contextWrapper()->takeContextOwnership(qmlglobal);

    if (!script->m_program.IsEmpty()) {
        script->m_program->Run(qmlglobal);
    } else {
        // Compilation failed.
        Q_ASSERT(try_catch.HasCaught());
    }

    if (try_catch.HasCaught()) {
        v8::Local<v8::Message> message = try_catch.Message();
        if (!message.IsEmpty()) {
            QQmlError error;
            QQmlExpressionPrivate::exceptionToError(message, error);
            ep->warning(error);
        }
    } 

    rv = qPersistentNew<v8::Object>(qmlglobal);
    if (shared) {
        script->m_value = qPersistentNew<v8::Object>(qmlglobal);
        script->m_loaded = true;
    }

    return rv;
}

#ifdef QML_THREADED_VME_INTERPRETER
void *const *QQmlVME::instructionJumpTable()
{
    static void * const *jumpTable = 0;
    if (!jumpTable) {
        QQmlVME dummy;
        QQmlVME::Interrupt i;
        dummy.run(0, i, &jumpTable);
    }
    return jumpTable;
}
#endif

QQmlContextData *QQmlVME::complete(const Interrupt &interrupt)
{
    Q_ASSERT(engine ||
             (bindValues.isEmpty() &&
              parserStatus.isEmpty() &&
              componentAttached == 0 &&
              rootContext.isNull() &&
              finalizeCallbacks.isEmpty()));

    if (!engine)
        return 0;

    QQmlTrace trace("VME Complete");
#ifdef QML_ENABLE_TRACE
    trace.addDetail("URL", rootComponent->url);
#endif

    ActiveVMERestorer restore(this, QQmlEnginePrivate::get(engine));
    QRecursionWatcher<QQmlVME, &QQmlVME::recursion> watcher(this);

    {
    QQmlTrace trace("VME Binding Enable");
    trace.event("begin binding eval");
    while (!bindValues.isEmpty()) {
        QQmlAbstractBinding *b = bindValues.pop();

        if (b) {
            b->m_mePtr = 0;
            QQmlData *data = QQmlData::get(b->object());
            Q_ASSERT(data);
            data->clearPendingBindingBit(b->propertyIndex());
            b->setEnabled(true, QQmlPropertyPrivate::BypassInterceptor |
                                QQmlPropertyPrivate::DontRemoveBinding);
        }

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
    }
    bindValues.deallocate();
    }

    if (componentCompleteEnabled()) { // the qml designer does the component complete later
        QQmlTrace trace("VME Component Complete");
        while (!parserStatus.isEmpty()) {
            QQmlParserStatus *status = parserStatus.pop();
#ifdef QML_ENABLE_TRACE
            QQmlData *data = parserStatusData.pop();
#endif

            if (status && status->d) {
                status->d = 0;
#ifdef QML_ENABLE_TRACE
                QQmlTrace trace("Component complete");
                trace.addDetail("URL", data->outerContext->url);
                trace.addDetail("Line", data->lineNumber);
#endif
                status->componentComplete();
            }

            if (watcher.hasRecursed() || interrupt.shouldInterrupt())
                return 0;
        }
        parserStatus.deallocate();
    }

    {
    QQmlTrace trace("VME Finalize Callbacks");
    for (int ii = 0; ii < finalizeCallbacks.count(); ++ii) {
        QQmlEnginePrivate::FinalizeCallback callback = finalizeCallbacks.at(ii);
        QObject *obj = callback.first;
        if (obj) {
            void *args[] = { 0 };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, callback.second, args);
        }
        if (watcher.hasRecursed())
            return 0;
    }
    finalizeCallbacks.clear();
    }

    {
    QQmlTrace trace("VME Component.onCompleted Callbacks");
    while (componentAttached) {
        QQmlComponentAttached *a = componentAttached;
        a->rem();
        QQmlData *d = QQmlData::get(a->parent());
        Q_ASSERT(d);
        Q_ASSERT(d->context);
        a->add(&d->context->componentAttached);
        if (componentCompleteEnabled())
            emit a->completed();

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
    }
    }

    QQmlContextData *rv = rootContext;

    reset();

    if (rv) rv->activeVMEData = data;

    return rv;
}

void QQmlVME::enableComponentComplete()
{
    s_enableComponentComplete = true;
}

void QQmlVME::disableComponentComplete()
{
    s_enableComponentComplete = false;
}

bool QQmlVME::componentCompleteEnabled()
{
    return s_enableComponentComplete;
}

void QQmlVME::blank(QFiniteStack<QQmlAbstractBinding *> &bs)
{
    for (int ii = 0; ii < bs.count(); ++ii) {
        QQmlAbstractBinding *b = bs.at(ii);
        if (b) b->m_mePtr = 0;
    }
}

void QQmlVME::blank(QFiniteStack<QQmlParserStatus *> &pss)
{
    for (int ii = 0; ii < pss.count(); ++ii) {
        QQmlParserStatus *ps = pss.at(ii);
        if(ps) ps->d = 0;
    }
}

QQmlVMEGuard::QQmlVMEGuard()
: m_objectCount(0), m_objects(0), m_contextCount(0), m_contexts(0)
{
}

QQmlVMEGuard::~QQmlVMEGuard()
{
    clear();
}

void QQmlVMEGuard::guard(QQmlVME *vme)
{
    clear();
    
    m_objectCount = vme->objects.count();
    m_objects = new QQmlGuard<QObject>[m_objectCount];
    for (int ii = 0; ii < m_objectCount; ++ii)
        m_objects[ii] = vme->objects[ii];

    m_contextCount = (vme->rootContext.isNull()?0:1) + vme->states.count();
    m_contexts = new QQmlGuardedContextData[m_contextCount];
    for (int ii = 0; ii < vme->states.count(); ++ii) 
        m_contexts[ii] = vme->states.at(ii).context;
    if (!vme->rootContext.isNull())
        m_contexts[m_contextCount - 1] = vme->rootContext.contextData();
}

void QQmlVMEGuard::clear()
{
    delete [] m_objects;
    delete [] m_contexts;

    m_objectCount = 0;
    m_objects = 0;
    m_contextCount = 0;
    m_contexts = 0;
}

bool QQmlVMEGuard::isOK() const
{
    for (int ii = 0; ii < m_objectCount; ++ii)
        if (m_objects[ii].isNull())
            return false;

    for (int ii = 0; ii < m_contextCount; ++ii)
        if (m_contexts[ii].isNull() || !m_contexts[ii]->engine)
            return false;

    return true;
}

QT_END_NAMESPACE
