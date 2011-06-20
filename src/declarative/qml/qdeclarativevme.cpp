/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "private/qdeclarativevme_p.h"

#include "private/qdeclarativecompiler_p.h"
#include "private/qdeclarativeboundsignal_p.h"
#include "private/qdeclarativestringconverters_p.h"
#include "private/qmetaobjectbuilder_p.h"
#include "private/qdeclarativedata_p.h"
#include "qdeclarative.h"
#include "private/qdeclarativecustomparser_p.h"
#include "qdeclarativeengine.h"
#include "qdeclarativecontext.h"
#include "qdeclarativecomponent.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qdeclarativeengine_p.h"
#include "private/qdeclarativecomponent_p.h"
#include "private/qdeclarativevmemetaobject_p.h"
#include "private/qdeclarativebinding_p_p.h"
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativev4bindings_p.h"
#include "private/qv8bindings_p.h"
#include "private/qdeclarativeglobal_p.h"
#include "qdeclarativescriptstring.h"
#include "qdeclarativescriptstring_p.h"

#include <QStack>
#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatetime.h>
#include <QtScript/qscriptvalue.h>

QT_BEGIN_NAMESPACE

QDeclarativeVME::QDeclarativeVME()
{
}

#define VME_EXCEPTION(desc, line) \
    { \
        QDeclarativeError error; \
        error.setDescription(desc.trimmed()); \
        error.setLine(line); \
        error.setUrl(comp->url); \
        vmeErrors << error; \
        break; \
    }

struct ListInstance
{
    ListInstance() 
        : type(0) {}
    ListInstance(int t) 
        : type(t) {}

    int type;
    QDeclarativeListProperty<void> qListProperty;
};

QObject *QDeclarativeVME::run(QDeclarativeContextData *ctxt, QDeclarativeCompiledData *comp, 
                              int start, const QBitField &bindingSkipList)
{
    QDeclarativeVMEStack<QObject *> stack;

    if (start == -1) start = 0;

    return run(stack, ctxt, comp, start, bindingSkipList);
}

void QDeclarativeVME::runDeferred(QObject *object)
{
    QDeclarativeData *data = QDeclarativeData::get(object);

    if (!data || !data->context || !data->deferredComponent)
        return;

    QDeclarativeContextData *ctxt = data->context;
    QDeclarativeCompiledData *comp = data->deferredComponent;
    int start = data->deferredIdx;
    QDeclarativeVMEStack<QObject *> stack;
    stack.push(object);

    run(stack, ctxt, comp, start, QBitField());
}

inline bool fastHasBinding(QObject *o, int index) 
{
    QDeclarativeData *ddata = static_cast<QDeclarativeData *>(QObjectPrivate::get(o)->declarativeData);

    return ddata && (ddata->bindingBitsSize > index) && 
           (ddata->bindingBits[index / 32] & (1 << (index % 32)));
}

static void removeBindingOnProperty(QObject *o, int index)
{
    QDeclarativeAbstractBinding *binding = QDeclarativePropertyPrivate::setBinding(o, index, -1, 0);
    if (binding) binding->destroy();
}

#define QML_BEGIN_INSTR(I) \
    case QDeclarativeInstruction::I: { \
        const QDeclarativeInstructionMeta<(int)QDeclarativeInstruction::I>::DataType &instr = QDeclarativeInstructionMeta<(int)QDeclarativeInstruction::I>::data(genericInstr); \
        instructionStream += QDeclarativeInstructionMeta<(int)QDeclarativeInstruction::I>::Size; \
        Q_UNUSED(instr); 

#define QML_END_INSTR(I) } break;

#define CLEAN_PROPERTY(o, index) if (fastHasBinding(o, index)) removeBindingOnProperty(o, index)

QObject *QDeclarativeVME::run(QDeclarativeVMEStack<QObject *> &stack, 
                              QDeclarativeContextData *ctxt, 
                              QDeclarativeCompiledData *comp, 
                              int start, const QBitField &bindingSkipList)
{
    Q_ASSERT(comp);
    Q_ASSERT(ctxt);
    const QList<QDeclarativeCompiledData::TypeReference> &types = comp->types;
    const QList<QString> &primitives = comp->primitives;
    const QList<QByteArray> &datas = comp->datas;
    const QList<QDeclarativePropertyCache *> &propertyCaches = comp->propertyCaches;
    const QList<QDeclarativeScriptData *> &scripts = comp->scripts;
    const QList<QUrl> &urls = comp->urls;

    QDeclarativeEnginePrivate::SimpleList<QDeclarativeAbstractBinding> bindValues;
    QDeclarativeEnginePrivate::SimpleList<QDeclarativeParserStatus> parserStatus;

    QDeclarativeVMEStack<ListInstance> qliststack;

    vmeErrors.clear();
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(ctxt->engine);

    int status = -1;    //for dbus
    QDeclarativePropertyPrivate::WriteFlags flags = QDeclarativePropertyPrivate::BypassInterceptor |
                                                    QDeclarativePropertyPrivate::RemoveBindingOnAliasWrite;

    const char *instructionStream = comp->bytecode.constData() + start;

    bool done = false;
    while (!isError() && !done) {
        const QDeclarativeInstruction &genericInstr = *((QDeclarativeInstruction *)instructionStream);

        switch(genericInstr.type()) {
        QML_BEGIN_INSTR(Init)
            if (instr.bindingsSize) 
                bindValues = QDeclarativeEnginePrivate::SimpleList<QDeclarativeAbstractBinding>(instr.bindingsSize);
            if (instr.parserStatusSize)
                parserStatus = QDeclarativeEnginePrivate::SimpleList<QDeclarativeParserStatus>(instr.parserStatusSize);
            if (instr.contextCache != -1) 
                ctxt->setIdPropertyData(comp->contextCaches.at(instr.contextCache));
            if (instr.compiledBinding != -1) {
                const char *v4data = datas.at(instr.compiledBinding).constData();
                ctxt->v4bindings = new QDeclarativeV4Bindings(v4data, ctxt);
            }
        QML_END_INSTR(Init)

        QML_BEGIN_INSTR(Done)
            done = true;
        QML_END_INSTR(Done)

        QML_BEGIN_INSTR(CreateObject)
            QBitField bindings;
            if (instr.bindingBits != -1) {
                const QByteArray &bits = datas.at(instr.bindingBits);
                bindings = QBitField((const quint32*)bits.constData(),
                                     bits.size() * 8);
            }
            if (stack.isEmpty())
                bindings = bindings.united(bindingSkipList);

            QObject *o = 
                types.at(instr.type).createInstance(ctxt, bindings, &vmeErrors);

            if (!o) {
                VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Unable to create object of type %1").arg(QString::fromLatin1(types.at(instr.type).className)), instr.line);
            }

            QDeclarativeData *ddata = QDeclarativeData::get(o);
            Q_ASSERT(ddata);

            if (stack.isEmpty()) {
                if (ddata->context) {
                    Q_ASSERT(ddata->context != ctxt);
                    Q_ASSERT(ddata->outerContext);
                    Q_ASSERT(ddata->outerContext != ctxt);
                    QDeclarativeContextData *c = ddata->context;
                    while (c->linkedContext) c = c->linkedContext;
                    c->linkedContext = ctxt;
                } else {
                    ctxt->addObject(o);
                }

                ddata->ownContext = true;
            } else if (!ddata->context) {
                ctxt->addObject(o);
            }

            ddata->setImplicitDestructible();
            ddata->outerContext = ctxt;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            if (instr.data != -1) {
                QDeclarativeCustomParser *customParser =
                    types.at(instr.type).type->customParser();
                customParser->setCustomData(o, datas.at(instr.data));
            }
            if (!stack.isEmpty()) {
                QObject *parent = stack.top();
                if (o->isWidgetType()) { 
                    QWidget *widget = static_cast<QWidget*>(o); 
                    if (parent->isWidgetType()) { 
                        QWidget *parentWidget = static_cast<QWidget*>(parent); 
                        widget->setParent(parentWidget); 
                    } else { 
                        // TODO: parent might be a layout 
                    } 
                } else { 
                        QDeclarative_setParent_noEvent(o, parent);
                } 
            }
            stack.push(o);
        QML_END_INSTR(CreateObject)

        QML_BEGIN_INSTR(CreateSimpleObject)
            QObject *o = (QObject *)operator new(instr.typeSize + sizeof(QDeclarativeData));   
            ::memset(o, 0, instr.typeSize + sizeof(QDeclarativeData));
            instr.create(o);

            QDeclarativeData *ddata = (QDeclarativeData *)(((const char *)o) + instr.typeSize);
            const QDeclarativeCompiledData::TypeReference &ref = types.at(instr.type);
            if (!ddata->propertyCache && ref.typePropertyCache) {
                ddata->propertyCache = ref.typePropertyCache;
                ddata->propertyCache->addref();
            }
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            QObjectPrivate::get(o)->declarativeData = ddata;                                                      
            ddata->context = ddata->outerContext = ctxt;
            ddata->nextContextObject = ctxt->contextObjects; 
            if (ddata->nextContextObject) 
                ddata->nextContextObject->prevContextObject = &ddata->nextContextObject; 
            ddata->prevContextObject = &ctxt->contextObjects; 
            ctxt->contextObjects = ddata; 

            QObject *parent = stack.top();                                                                    
            QDeclarative_setParent_noEvent(o, parent);                                                        

            stack.push(o);
        QML_END_INSTR(CreateSimpleObject)

        QML_BEGIN_INSTR(SetId)
            QObject *target = stack.top();
            ctxt->setIdProperty(instr.index, target);
        QML_END_INSTR(SetId)

        QML_BEGIN_INSTR(SetDefault)
            ctxt->contextObject = stack.top();
        QML_END_INSTR(SetDefault)

        QML_BEGIN_INSTR(CreateComponent)
            QDeclarativeComponent *qcomp = 
                new QDeclarativeComponent(ctxt->engine, comp, instructionStream - comp->bytecode.constData(),
                                          stack.isEmpty() ? 0 : stack.top());

            QDeclarativeData *ddata = QDeclarativeData::get(qcomp, true);
            Q_ASSERT(ddata);

            ctxt->addObject(qcomp);

            if (stack.isEmpty()) 
                ddata->ownContext = true;

            ddata->setImplicitDestructible();
            ddata->outerContext = ctxt;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            QDeclarativeComponentPrivate::get(qcomp)->creationContext = ctxt;

            stack.push(qcomp);
            instructionStream += instr.count;
        QML_END_INSTR(CreateComponent)

        QML_BEGIN_INSTR(StoreMetaObject)
            QObject *target = stack.top();

            QMetaObject mo;
            const QByteArray &metadata = datas.at(instr.data);
            QMetaObjectBuilder::fromRelocatableData(&mo, 0, metadata);

            const QDeclarativeVMEMetaData *data = 
                (const QDeclarativeVMEMetaData *)datas.at(instr.aliasData).constData();

            (void)new QDeclarativeVMEMetaObject(target, &mo, data, comp);

            if (instr.propertyCache != -1) {
                QDeclarativeData *ddata = QDeclarativeData::get(target, true);
                if (ddata->propertyCache) ddata->propertyCache->release();
                ddata->propertyCache = propertyCaches.at(instr.propertyCache);
                ddata->propertyCache->addref();
            }
        QML_END_INSTR(StoreMetaObject)

        QML_BEGIN_INSTR(StoreVariant)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            // XXX - can be more efficient
            QVariant v = QDeclarativeStringConverters::variantFromString(primitives.at(instr.value));
            void *a[] = { &v, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreVariant)

        QML_BEGIN_INSTR(StoreVariantInteger)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QVariant v(instr.value);
            void *a[] = { &v, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreVariantInteger)

        QML_BEGIN_INSTR(StoreVariantDouble)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QVariant v(instr.value);
            void *a[] = { &v, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreVariantDouble)

        QML_BEGIN_INSTR(StoreVariantBool)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QVariant v(instr.value);
            void *a[] = { &v, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreVariantBool)

        QML_BEGIN_INSTR(StoreString)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            void *a[] = { (void *)&primitives.at(instr.value), 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreString)

        QML_BEGIN_INSTR(StoreByteArray)
            QObject *target = stack.top();
            void *a[] = { (void *)&datas.at(instr.value), 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty,
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreByteArray)

        QML_BEGIN_INSTR(StoreUrl)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            void *a[] = { (void *)&urls.at(instr.value), 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreUrl)

        QML_BEGIN_INSTR(StoreFloat)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            float f = instr.value;
            void *a[] = { &f, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty,
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreFloat)

        QML_BEGIN_INSTR(StoreDouble)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            double d = instr.value;
            void *a[] = { &d, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty,
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreDouble)

        QML_BEGIN_INSTR(StoreBool)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            void *a[] = { (void *)&instr.value, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreBool)

        QML_BEGIN_INSTR(StoreInteger)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            void *a[] = { (void *)&instr.value, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreInteger)

        QML_BEGIN_INSTR(StoreColor)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QColor c = QColor::fromRgba(instr.value);
            void *a[] = { &c, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreColor)

        QML_BEGIN_INSTR(StoreDate)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QDate d = QDate::fromJulianDay(instr.value);
            void *a[] = { &d, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreDate)

        QML_BEGIN_INSTR(StoreTime)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QTime *t = (QTime *)&instr.time;
            void *a[] = { t, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreTime)

        QML_BEGIN_INSTR(StoreDateTime)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QTime *t = (QTime *)&instr.time;
            QDateTime dt(QDate::fromJulianDay(instr.date), *t);
            void *a[] = { &dt, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty,
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreDateTime)

        QML_BEGIN_INSTR(StorePoint)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QPoint *p = (QPoint *)&instr.point;
            void *a[] = { p, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StorePoint)

        QML_BEGIN_INSTR(StorePointF)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QPointF *p = (QPointF *)&instr.point;
            void *a[] = { p, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StorePointF)

        QML_BEGIN_INSTR(StoreSize)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QSize *s = (QSize *)&instr.size;
            void *a[] = { s, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreSize)

        QML_BEGIN_INSTR(StoreSizeF)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QSizeF *s = (QSizeF *)&instr.size;
            void *a[] = { s, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreSizeF)

        QML_BEGIN_INSTR(StoreRect)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QRect *r = (QRect *)&instr.rect;
            void *a[] = { r, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreRect)

        QML_BEGIN_INSTR(StoreRectF)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QRectF *r = (QRectF *)&instr.rect;
            void *a[] = { r, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreRectF)

        QML_BEGIN_INSTR(StoreVector3D)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QVector3D *v = (QVector3D *)&instr.vector;
            void *a[] = { v, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreVector3D)

        QML_BEGIN_INSTR(StoreObject)
            QObject *assignObj = stack.pop();
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            void *a[] = { (void *)&assignObj, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreObject)

        QML_BEGIN_INSTR(AssignCustomType)
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            const QString &primitive = primitives.at(instr.primitive);
            int type = instr.type;
            QDeclarativeMetaType::StringConverter converter = QDeclarativeMetaType::customStringConverter(type);
            QVariant v = (*converter)(primitive);

            QMetaProperty prop = 
                    target->metaObject()->property(instr.propertyIndex);
            if (v.isNull() || ((int)prop.type() != type && prop.userType() != type)) 
                VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Cannot assign value %1 to property %2").arg(primitive).arg(QString::fromUtf8(prop.name())), instr.line);

            void *a[] = { (void *)v.data(), 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(AssignCustomType)

        QML_BEGIN_INSTR(AssignSignalObject)
            // XXX optimize

            QObject *assign = stack.pop();
            QObject *target = stack.top();
            int sigIdx = instr.signal;
            const QByteArray &pr = datas.at(sigIdx);

            QDeclarativeProperty prop(target, QString::fromUtf8(pr));
            if (prop.type() & QDeclarativeProperty::SignalProperty) {

                QMetaMethod method = QDeclarativeMetaType::defaultMethod(assign);
                if (method.signature() == 0)
                    VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Cannot assign object type %1 with no default method").arg(QString::fromLatin1(assign->metaObject()->className())), instr.line);

                if (!QMetaObject::checkConnectArgs(prop.method().signature(), method.signature()))
                    VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Cannot connect mismatched signal/slot %1 %vs. %2").arg(QString::fromLatin1(method.signature())).arg(QString::fromLatin1(prop.method().signature())), instr.line);

                QDeclarativePropertyPrivate::connect(target, prop.index(), assign, method.methodIndex());

            } else {
                VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Cannot assign an object to signal property %1").arg(QString::fromUtf8(pr)), instr.line);
            }


        QML_END_INSTR(AssignSignalObject)

        QML_BEGIN_INSTR(StoreSignal)
            QObject *target = stack.top();
            QObject *context = stack.at(stack.count() - 1 - instr.context);

            QMetaMethod signal = target->metaObject()->method(instr.signalIndex);

            QDeclarativeBoundSignal *bs = new QDeclarativeBoundSignal(target, signal, target);
            QDeclarativeExpression *expr = 
                new QDeclarativeExpression(ctxt, context, primitives.at(instr.value));
            expr->setSourceLocation(comp->name, instr.line);
            static_cast<QDeclarativeExpressionPrivate *>(QObjectPrivate::get(expr))->name = datas.at(instr.name);
            bs->setExpression(expr);
        QML_END_INSTR(StoreSignal)

        QML_BEGIN_INSTR(StoreImportedScript)
            ctxt->importedScripts << run(ctxt, scripts.at(instr.value));
        QML_END_INSTR(StoreImportedScript)

        QML_BEGIN_INSTR(StoreScriptString)
            QObject *target = stack.top();
            QObject *scope = stack.at(stack.count() - 1 - instr.scope);
            QDeclarativeScriptString ss;
            ss.setContext(ctxt->asQDeclarativeContext());
            ss.setScopeObject(scope);
            ss.setScript(primitives.at(instr.value));
            ss.d.data()->bindingId = instr.bindingId;
            ss.d.data()->lineNumber = instr.line;

            void *a[] = { &ss, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreScriptString)

        QML_BEGIN_INSTR(BeginObject)
            QObject *target = stack.top();
            QDeclarativeParserStatus *status = reinterpret_cast<QDeclarativeParserStatus *>(reinterpret_cast<char *>(target) + instr.castValue);
            parserStatus.append(status);
            status->d = &parserStatus.values[parserStatus.count - 1];

            status->classBegin();
        QML_END_INSTR(BeginObject)

        QML_BEGIN_INSTR(InitV8Bindings)
            ctxt->v8bindings = new QV8Bindings(primitives.at(instr.program), instr.programIndex, 
                                               instr.line, comp, ctxt);
        QML_END_INSTR(InitV8Bindings)

        QML_BEGIN_INSTR(StoreBinding)
            QObject *target = 
                stack.at(stack.count() - 1 - instr.owner);
            QObject *context = 
                stack.at(stack.count() - 1 - instr.context);

            QDeclarativeProperty mp = 
                QDeclarativePropertyPrivate::restore(datas.at(instr.property), target, ctxt);

            int coreIndex = mp.index();

            if ((stack.count() - instr.owner) == 1 && bindingSkipList.testBit(coreIndex)) 
                break;

            QDeclarativeBinding *bind = new QDeclarativeBinding(primitives.at(instr.value), true, 
                                                                context, ctxt, comp->name, instr.line);
            bindValues.append(bind);
            bind->m_mePtr = &bindValues.values[bindValues.count - 1];
            bind->setTarget(mp);

            bind->addToObject(target, QDeclarativePropertyPrivate::bindingIndex(mp));
        QML_END_INSTR(StoreBinding)

        QML_BEGIN_INSTR(StoreBindingOnAlias)
            QObject *target = 
                stack.at(stack.count() - 1 - instr.owner);
            QObject *context = 
                stack.at(stack.count() - 1 - instr.context);

            QDeclarativeProperty mp = 
                QDeclarativePropertyPrivate::restore(datas.at(instr.property), target, ctxt);

            int coreIndex = mp.index();

            if ((stack.count() - instr.owner) == 1 && bindingSkipList.testBit(coreIndex)) 
                break;

            QDeclarativeBinding *bind = new QDeclarativeBinding(primitives.at(instr.value), true,
                                                                context, ctxt, comp->name, instr.line);
            bindValues.append(bind);
            bind->m_mePtr = &bindValues.values[bindValues.count - 1];
            bind->setTarget(mp);

            QDeclarativeAbstractBinding *old = QDeclarativePropertyPrivate::setBindingNoEnable(target, coreIndex, QDeclarativePropertyPrivate::valueTypeCoreIndex(mp), bind);
            if (old) { old->destroy(); }
        QML_END_INSTR(StoreBindingOnAlias)

        QML_BEGIN_INSTR(StoreV4Binding)
            QObject *target = 
                stack.at(stack.count() - 1 - instr.owner);
            QObject *scope = 
                stack.at(stack.count() - 1 - instr.context);

            int property = instr.property;
            if (stack.count() == 1 && bindingSkipList.testBit(property & 0xFFFF))  
                break;

            QDeclarativeAbstractBinding *binding = 
                ctxt->v4bindings->configBinding(instr.value, target, scope, property);
            bindValues.append(binding);
            binding->m_mePtr = &bindValues.values[bindValues.count - 1];
            binding->addToObject(target, property);
        QML_END_INSTR(StoreV4Binding)

        QML_BEGIN_INSTR(StoreV8Binding)
            QObject *target = 
                stack.at(stack.count() - 1 - instr.owner);
            QObject *scope = 
                stack.at(stack.count() - 1 - instr.context);

            QDeclarativeProperty mp = 
                QDeclarativePropertyPrivate::restore(datas.at(instr.property), target, ctxt);

            int coreIndex = mp.index();

            if ((stack.count() - instr.owner) == 1 && bindingSkipList.testBit(coreIndex))
                break;

            QDeclarativeAbstractBinding *binding = 
                ctxt->v8bindings->configBinding(instr.value, target, scope, mp, instr.line);
            bindValues.append(binding);
            binding->m_mePtr = &bindValues.values[bindValues.count - 1];
            binding->addToObject(target, QDeclarativePropertyPrivate::bindingIndex(mp));
        QML_END_INSTR(StoreV8Binding)

        QML_BEGIN_INSTR(StoreValueSource)
            QObject *obj = stack.pop();
            QDeclarativePropertyValueSource *vs = reinterpret_cast<QDeclarativePropertyValueSource *>(reinterpret_cast<char *>(obj) + instr.castValue);
            QObject *target = stack.at(stack.count() - 1 - instr.owner);

            QDeclarativeProperty prop = 
                QDeclarativePropertyPrivate::restore(datas.at(instr.property), target, ctxt);
            obj->setParent(target);
            vs->setTarget(prop);
        QML_END_INSTR(StoreValueSource)

        QML_BEGIN_INSTR(StoreValueInterceptor)
            QObject *obj = stack.pop();
            QDeclarativePropertyValueInterceptor *vi = reinterpret_cast<QDeclarativePropertyValueInterceptor *>(reinterpret_cast<char *>(obj) + instr.castValue);
            QObject *target = stack.at(stack.count() - 1 - instr.owner);
            QDeclarativeProperty prop = 
                QDeclarativePropertyPrivate::restore(datas.at(instr.property), target, ctxt);
            obj->setParent(target);
            vi->setTarget(prop);
            QDeclarativeVMEMetaObject *mo = static_cast<QDeclarativeVMEMetaObject *>((QMetaObject*)target->metaObject());
            mo->registerInterceptor(prop.index(), QDeclarativePropertyPrivate::valueTypeCoreIndex(prop), vi);
        QML_END_INSTR(StoreValueInterceptor)

        QML_BEGIN_INSTR(StoreObjectQList)
            QObject *assign = stack.pop();

            const ListInstance &list = qliststack.top();
            list.qListProperty.append((QDeclarativeListProperty<void>*)&list.qListProperty, assign);
        QML_END_INSTR(StoreObjectQList)

        QML_BEGIN_INSTR(AssignObjectList)
            // This is only used for assigning interfaces
            QObject *assign = stack.pop();
            const ListInstance &list = qliststack.top();

            int type = list.type;

            void *ptr = 0;

            const char *iid = QDeclarativeMetaType::interfaceIId(type);
            if (iid) 
                ptr = assign->qt_metacast(iid);
            if (!ptr) 
                VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Cannot assign object to list"), instr.line);


            list.qListProperty.append((QDeclarativeListProperty<void>*)&list.qListProperty, ptr);
        QML_END_INSTR(AssignObjectList)

        QML_BEGIN_INSTR(StoreVariantObject)
            QObject *assign = stack.pop();
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            QVariant v = QVariant::fromValue(assign);
            void *a[] = { &v, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreVariantObject)

        QML_BEGIN_INSTR(StoreInterface)
            QObject *assign = stack.pop();
            QObject *target = stack.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            int coreIdx = instr.propertyIndex;
            QMetaProperty prop = target->metaObject()->property(coreIdx);
            int t = prop.userType();
            const char *iid = QDeclarativeMetaType::interfaceIId(t);
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
                VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Cannot assign object to interface property"), instr.line);
        QML_END_INSTR(StoreInterface)
            
        QML_BEGIN_INSTR(FetchAttached)
            QObject *target = stack.top();

            QObject *qmlObject = qmlAttachedPropertiesObjectById(instr.id, target);

            if (!qmlObject)
                VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Unable to create attached object"), instr.line);

            stack.push(qmlObject);
        QML_END_INSTR(FetchAttached)

        QML_BEGIN_INSTR(FetchQList)
            QObject *target = stack.top();

            qliststack.push(ListInstance(instr.type));

            void *a[1];
            a[0] = (void *)&(qliststack.top().qListProperty);
            QMetaObject::metacall(target, QMetaObject::ReadProperty, 
                                  instr.property, a);
        QML_END_INSTR(FetchQList)

        QML_BEGIN_INSTR(FetchObject)
            QObject *target = stack.top();

            QObject *obj = 0;
            // NOTE: This assumes a cast to QObject does not alter the 
            // object pointer
            void *a[1];
            a[0] = &obj;
            QMetaObject::metacall(target, QMetaObject::ReadProperty, 
                                  instr.property, a);

            if (!obj)
                VME_EXCEPTION(QCoreApplication::translate("QDeclarativeVME","Cannot set properties on %1 as it is null").arg(QString::fromUtf8(target->metaObject()->property(instr.property).name())), instr.line);

            stack.push(obj);
        QML_END_INSTR(FetchObject)

        QML_BEGIN_INSTR(PopQList)
            qliststack.pop();
        QML_END_INSTR(PopQList)

        QML_BEGIN_INSTR(Defer)
            if (instr.deferCount) {
                QObject *target = stack.top();
                QDeclarativeData *data = 
                    QDeclarativeData::get(target, true);
                comp->addref();
                data->deferredComponent = comp;
                data->deferredIdx = instructionStream - comp->bytecode.constData();
                instructionStream += instr.deferCount;
            }
        QML_END_INSTR(Defer)

        QML_BEGIN_INSTR(PopFetchedObject)
            stack.pop();
        QML_END_INSTR(PopFetchedObject)

        QML_BEGIN_INSTR(FetchValueType)
            QObject *target = stack.top();

            if (instr.bindingSkipList != 0) {
                // Possibly need to clear bindings
                QDeclarativeData *targetData = QDeclarativeData::get(target);
                if (targetData) {
                    QDeclarativeAbstractBinding *binding = 
                        QDeclarativePropertyPrivate::binding(target, instr.property, -1);

                    if (binding && binding->bindingType() != QDeclarativeAbstractBinding::ValueTypeProxy) {
                        QDeclarativePropertyPrivate::setBinding(target, instr.property, -1, 0);
                        binding->destroy();
                    } else if (binding) {
                        QDeclarativeValueTypeProxyBinding *proxy = 
                            static_cast<QDeclarativeValueTypeProxyBinding *>(binding);
                        proxy->removeBindings(instr.bindingSkipList);
                    }
                }
            }

            QDeclarativeValueType *valueHandler = ep->valueTypes[instr.type];
            valueHandler->read(target, instr.property);
            stack.push(valueHandler);
        QML_END_INSTR(FetchValueType)

        QML_BEGIN_INSTR(PopValueType)
            QDeclarativeValueType *valueHandler = 
                static_cast<QDeclarativeValueType *>(stack.pop());
            QObject *target = stack.top();
            valueHandler->write(target, instr.property, QDeclarativePropertyPrivate::BypassInterceptor);
        QML_END_INSTR(PopValueType)

        default:
            qFatal("QDeclarativeCompiledData: Internal error - unknown instruction %d", genericInstr.type());
            break;
        }
    }

    if (isError()) {
        if (!stack.isEmpty()) {
            delete stack.at(0); // ### What about failures in deferred creation?
        } else {
            ctxt->destroy();
        }

        QDeclarativeEnginePrivate::clear(bindValues);
        QDeclarativeEnginePrivate::clear(parserStatus);
        return 0;
    }

    if (bindValues.count)
        ep->bindValues << bindValues;
    else if (bindValues.values)
        bindValues.clear();

    if (parserStatus.count)
        ep->parserStatus << parserStatus;
    else if (parserStatus.values)
        parserStatus.clear();

    Q_ASSERT(stack.count() == 1);
    return stack.top();
}

bool QDeclarativeVME::isError() const
{
    return !vmeErrors.isEmpty();
}

QList<QDeclarativeError> QDeclarativeVME::errors() const
{
    return vmeErrors;
}

QObject *
QDeclarativeCompiledData::TypeReference::createInstance(QDeclarativeContextData *ctxt,
                                                        const QBitField &bindings,
                                                        QList<QDeclarativeError> *errors) const
{
    if (type) {
        QObject *rv = 0;
        void *memory = 0;

        type->create(&rv, &memory, sizeof(QDeclarativeData));
        QDeclarativeData *ddata = new (memory) QDeclarativeData;
        ddata->ownMemory = false;
        QObjectPrivate::get(rv)->declarativeData = ddata;

        if (typePropertyCache && !ddata->propertyCache) {
            ddata->propertyCache = typePropertyCache;
            ddata->propertyCache->addref();
        }

        return rv;
    } else {
        Q_ASSERT(component);
        return QDeclarativeComponentPrivate::begin(ctxt, 0, component, -1, 0, errors, bindings);
    } 
}

v8::Persistent<v8::Object> QDeclarativeVME::run(QDeclarativeContextData *parentCtxt, QDeclarativeScriptData *script)
{
    if (script->m_loaded)
        return qPersistentNew<v8::Object>(script->m_value);

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(parentCtxt->engine);
    QV8Engine *v8engine = &ep->v8engine;

    bool shared = script->pragmas & QDeclarativeParser::Object::ScriptBlock::Shared;

    QDeclarativeContextData *effectiveCtxt = parentCtxt;
    if (shared)
        effectiveCtxt = 0;

    // Create the script context if required
    QDeclarativeContextData *ctxt = new QDeclarativeContextData;
    ctxt->isInternal = true;
    ctxt->isJSContext = true;
    if (shared)
        ctxt->isPragmaLibraryContext = true;
    else
        ctxt->isPragmaLibraryContext = parentCtxt->isPragmaLibraryContext;
    ctxt->url = script->url;

    // For backward compatibility, if there are no imports, we need to use the
    // imports from the parent context.  See QTBUG-17518.
    if (!script->importCache->isEmpty()) {
        ctxt->imports = script->importCache;
    } else if (effectiveCtxt) {
        ctxt->imports = effectiveCtxt->imports;
    }

    if (ctxt->imports) {
        ctxt->imports->addref();
    }

    if (effectiveCtxt)
        ctxt->setParent(effectiveCtxt, true);

    for (int ii = 0; ii < script->scripts.count(); ++ii) {
        ctxt->importedScripts << run(ctxt, script->scripts.at(ii)->scriptData());
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(v8engine->context());

    v8::Local<v8::Object> qmlglobal = v8engine->qmlScope(ctxt, 0);

    v8::TryCatch try_catch;
    script->m_program->Run(qmlglobal);

    v8::Persistent<v8::Object> rv;
    
    if (try_catch.HasCaught()) {
        v8::Local<v8::Message> message = try_catch.Message();
        if (!message.IsEmpty()) {
            QDeclarativeError error;
            QDeclarativeExpressionPrivate::exceptionToError(message, error);
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

QT_END_NAMESPACE
