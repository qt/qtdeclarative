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

#include "qdeclarativevmemetaobject_p.h"


#include "qdeclarative.h"
#include <private/qdeclarativerefcount_p.h>
#include "qdeclarativeexpression.h"
#include "qdeclarativeexpression_p.h"
#include "qdeclarativecontext_p.h"
#include "qdeclarativebinding_p.h"
#include "qdeclarativepropertyvalueinterceptor_p.h"

#include <private/qv8variantresource_p.h>

Q_DECLARE_METATYPE(QJSValue);

QT_BEGIN_NAMESPACE

class QDeclarativeVMEVariant
{
public:
    inline QDeclarativeVMEVariant();
    inline ~QDeclarativeVMEVariant();

    inline const void *dataPtr() const;
    inline void *dataPtr();
    inline int dataType() const;

    inline QObject *asQObject();
    inline const QVariant &asQVariant();
    inline int asInt();
    inline bool asBool();
    inline double asDouble();
    inline const QString &asQString();
    inline const QUrl &asQUrl();
    inline const QColor &asQColor();
    inline const QTime &asQTime();
    inline const QDate &asQDate();
    inline const QDateTime &asQDateTime();
    inline const QJSValue &asQJSValue();

    inline void setValue(QObject *);
    inline void setValue(const QVariant &);
    inline void setValue(int);
    inline void setValue(bool);
    inline void setValue(double);
    inline void setValue(const QString &);
    inline void setValue(const QUrl &);
    inline void setValue(const QColor &);
    inline void setValue(const QTime &);
    inline void setValue(const QDate &);
    inline void setValue(const QDateTime &);
    inline void setValue(const QJSValue &);
private:
    int type;
    void *data[4]; // Large enough to hold all types

    inline void cleanup();
};

class QDeclarativeVMEMetaObjectEndpoint : public QDeclarativeNotifierEndpoint
{
public:
    QDeclarativeVMEMetaObjectEndpoint();
    static void vmecallback(QDeclarativeNotifierEndpoint *);
    void tryConnect();

    QFlagPointer<QDeclarativeVMEMetaObject> metaObject;
};


QDeclarativeVMEVariant::QDeclarativeVMEVariant()
: type(QVariant::Invalid)
{
}

QDeclarativeVMEVariant::~QDeclarativeVMEVariant()
{
    cleanup();
}

void QDeclarativeVMEVariant::cleanup()
{
    if (type == QVariant::Invalid) {
    } else if (type == QMetaType::Int ||
               type == QMetaType::Bool ||
               type == QMetaType::Double) {
        type = QVariant::Invalid;
    } else if (type == QMetaType::QObjectStar) {
        ((QDeclarativeGuard<QObject>*)dataPtr())->~QDeclarativeGuard<QObject>();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QString) {
        ((QString *)dataPtr())->~QString();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QUrl) {
        ((QUrl *)dataPtr())->~QUrl();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QColor) {
        ((QColor *)dataPtr())->~QColor();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QTime) {
        ((QTime *)dataPtr())->~QTime();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QDate) {
        ((QDate *)dataPtr())->~QDate();
        type = QVariant::Invalid;
    } else if (type == QMetaType::QDateTime) {
        ((QDateTime *)dataPtr())->~QDateTime();
        type = QVariant::Invalid;
    } else if (type == qMetaTypeId<QVariant>()) {
        ((QVariant *)dataPtr())->~QVariant();
        type = QVariant::Invalid;
    } else if (type == qMetaTypeId<QJSValue>()) {
        ((QJSValue *)dataPtr())->~QJSValue();
        type = QVariant::Invalid;
    }

}

int QDeclarativeVMEVariant::dataType() const
{
    return type;
}

const void *QDeclarativeVMEVariant::dataPtr() const
{
    return &data;
}

void *QDeclarativeVMEVariant::dataPtr() 
{
    return &data;
}

QObject *QDeclarativeVMEVariant::asQObject() 
{
    if (type != QMetaType::QObjectStar) 
        setValue((QObject *)0);

    return *(QDeclarativeGuard<QObject> *)(dataPtr());
}

const QVariant &QDeclarativeVMEVariant::asQVariant() 
{
    if (type != QMetaType::QVariant)
        setValue(QVariant());

    return *(QVariant *)(dataPtr());
}

int QDeclarativeVMEVariant::asInt() 
{
    if (type != QMetaType::Int)
        setValue(int(0));

    return *(int *)(dataPtr());
}

bool QDeclarativeVMEVariant::asBool() 
{
    if (type != QMetaType::Bool)
        setValue(bool(false));

    return *(bool *)(dataPtr());
}

double QDeclarativeVMEVariant::asDouble() 
{
    if (type != QMetaType::Double)
        setValue(double(0));

    return *(double *)(dataPtr());
}

const QString &QDeclarativeVMEVariant::asQString() 
{
    if (type != QMetaType::QString)
        setValue(QString());

    return *(QString *)(dataPtr());
}

const QUrl &QDeclarativeVMEVariant::asQUrl() 
{
    if (type != QMetaType::QUrl)
        setValue(QUrl());

    return *(QUrl *)(dataPtr());
}

const QColor &QDeclarativeVMEVariant::asQColor() 
{
    if (type != QMetaType::QColor)
        setValue(QColor());

    return *(QColor *)(dataPtr());
}

const QTime &QDeclarativeVMEVariant::asQTime() 
{
    if (type != QMetaType::QTime)
        setValue(QTime());

    return *(QTime *)(dataPtr());
}

const QDate &QDeclarativeVMEVariant::asQDate() 
{
    if (type != QMetaType::QDate)
        setValue(QDate());

    return *(QDate *)(dataPtr());
}

const QDateTime &QDeclarativeVMEVariant::asQDateTime() 
{
    if (type != QMetaType::QDateTime)
        setValue(QDateTime());

    return *(QDateTime *)(dataPtr());
}

const QJSValue &QDeclarativeVMEVariant::asQJSValue()
{
    if (type != qMetaTypeId<QJSValue>())
        setValue(QJSValue());

    return *(QJSValue *)(dataPtr());
}

void QDeclarativeVMEVariant::setValue(QObject *v)
{
    if (type != QMetaType::QObjectStar) {
        cleanup();
        type = QMetaType::QObjectStar;
        new (dataPtr()) QDeclarativeGuard<QObject>();
    }
    *(QDeclarativeGuard<QObject>*)(dataPtr()) = v;
}

void QDeclarativeVMEVariant::setValue(const QVariant &v)
{
    if (type != qMetaTypeId<QVariant>()) {
        cleanup();
        type = qMetaTypeId<QVariant>();
        new (dataPtr()) QVariant(v);
    } else {
        *(QVariant *)(dataPtr()) = v;
    }
}

void QDeclarativeVMEVariant::setValue(int v)
{
    if (type != QMetaType::Int) {
        cleanup();
        type = QMetaType::Int;
    }
    *(int *)(dataPtr()) = v;
}

void QDeclarativeVMEVariant::setValue(bool v)
{
    if (type != QMetaType::Bool) {
        cleanup();
        type = QMetaType::Bool;
    }
    *(bool *)(dataPtr()) = v;
}

void QDeclarativeVMEVariant::setValue(double v)
{
    if (type != QMetaType::Double) {
        cleanup();
        type = QMetaType::Double;
    }
    *(double *)(dataPtr()) = v;
}

void QDeclarativeVMEVariant::setValue(const QString &v)
{
    if (type != QMetaType::QString) {
        cleanup();
        type = QMetaType::QString;
        new (dataPtr()) QString(v);
    } else {
        *(QString *)(dataPtr()) = v;
    }
}

void QDeclarativeVMEVariant::setValue(const QUrl &v)
{
    if (type != QMetaType::QUrl) {
        cleanup();
        type = QMetaType::QUrl;
        new (dataPtr()) QUrl(v);
    } else {
        *(QUrl *)(dataPtr()) = v;
    }
}

void QDeclarativeVMEVariant::setValue(const QColor &v)
{
    if (type != QMetaType::QColor) {
        cleanup();
        type = QMetaType::QColor;
        new (dataPtr()) QColor(v);
    } else {
        *(QColor *)(dataPtr()) = v;
    }
}

void QDeclarativeVMEVariant::setValue(const QTime &v)
{
    if (type != QMetaType::QTime) {
        cleanup();
        type = QMetaType::QTime;
        new (dataPtr()) QTime(v);
    } else {
        *(QTime *)(dataPtr()) = v;
    }
}

void QDeclarativeVMEVariant::setValue(const QDate &v)
{
    if (type != QMetaType::QDate) {
        cleanup();
        type = QMetaType::QDate;
        new (dataPtr()) QDate(v);
    } else {
        *(QDate *)(dataPtr()) = v;
    }
}

void QDeclarativeVMEVariant::setValue(const QDateTime &v)
{
    if (type != QMetaType::QDateTime) {
        cleanup();
        type = QMetaType::QDateTime;
        new (dataPtr()) QDateTime(v);
    } else {
        *(QDateTime *)(dataPtr()) = v;
    }
}

void QDeclarativeVMEVariant::setValue(const QJSValue &v)
{
    if (type != qMetaTypeId<QJSValue>()) {
        cleanup();
        type = qMetaTypeId<QJSValue>();
        new (dataPtr()) QJSValue(v);
    } else {
        *(QJSValue *)(dataPtr()) = v;
    }
}

QDeclarativeVMEMetaObjectEndpoint::QDeclarativeVMEMetaObjectEndpoint()
{
    callback = &vmecallback;
}

void QDeclarativeVMEMetaObjectEndpoint::vmecallback(QDeclarativeNotifierEndpoint *e)
{
    QDeclarativeVMEMetaObjectEndpoint *vmee = static_cast<QDeclarativeVMEMetaObjectEndpoint*>(e);
    vmee->tryConnect();
}

void QDeclarativeVMEMetaObjectEndpoint::tryConnect()
{
    if (metaObject.flag())
        return;

    int aliasId = this - metaObject->aliasEndpoints;

    QDeclarativeVMEMetaData::AliasData *d = metaObject->metaData->aliasData() + aliasId;
    if (!d->isObjectAlias()) {
        QDeclarativeContextData *ctxt = metaObject->ctxt;
        QObject *target = ctxt->idValues[d->contextIdx].data();
        if (!target)
            return;

        QMetaProperty prop = target->metaObject()->property(d->propertyIndex());
        if (prop.hasNotifySignal()) {
            int sigIdx = metaObject->methodOffset + aliasId + metaObject->metaData->propertyCount;
            QDeclarativePropertyPrivate::connect(target, prop.notifySignalIndex(),
                                                 metaObject->object, sigIdx);
        }
    }

    metaObject.setFlag();
}

QDeclarativeVMEMetaObject::QDeclarativeVMEMetaObject(QObject *obj,
                                                     const QMetaObject *other, 
                                                     const QDeclarativeVMEMetaData *meta,
                                                     QDeclarativeCompiledData *cdata)
: QV8GCCallback::Node(GcPrologueCallback), object(obj), compiledData(cdata),
  ctxt(QDeclarativeData::get(obj, true)->outerContext), metaData(meta), data(0),
  aliasEndpoints(0), firstVarPropertyIndex(-1), varPropertiesInitialized(false),
  v8methods(0), parent(0)
{
    compiledData->addref();

    *static_cast<QMetaObject *>(this) = *other;
    this->d.superdata = obj->metaObject();

    QObjectPrivate *op = QObjectPrivate::get(obj);
    if (op->metaObject)
        parent = static_cast<QAbstractDynamicMetaObject*>(op->metaObject);
    op->metaObject = this;

    propOffset = QAbstractDynamicMetaObject::propertyOffset();
    methodOffset = QAbstractDynamicMetaObject::methodOffset();

    data = new QDeclarativeVMEVariant[metaData->propertyCount - metaData->varPropertyCount];

    aConnected.resize(metaData->aliasCount);
    int list_type = qMetaTypeId<QDeclarativeListProperty<QObject> >();

    // ### Optimize
    for (int ii = 0; ii < metaData->propertyCount - metaData->varPropertyCount; ++ii) {
        int t = (metaData->propertyData() + ii)->propertyType;
        if (t == list_type) {
            listProperties.append(List(methodOffset + ii));
            data[ii].setValue(listProperties.count() - 1);
        } 
    }

    firstVarPropertyIndex = metaData->propertyCount - metaData->varPropertyCount;
    if (metaData->varPropertyCount)
        QV8GCCallback::addGcCallbackNode(this);
}

QDeclarativeVMEMetaObject::~QDeclarativeVMEMetaObject()
{
    compiledData->release();
    delete parent;
    delete [] data;
    delete [] aliasEndpoints;

    for (int ii = 0; v8methods && ii < metaData->methodCount; ++ii) {
        qPersistentDispose(v8methods[ii]);
    }

    if (metaData->varPropertyCount)
        qPersistentDispose(varProperties); // if not weak, will not have been cleaned up by the callback.
}

int QDeclarativeVMEMetaObject::metaCall(QMetaObject::Call c, int _id, void **a)
{
    int id = _id;
    if(c == QMetaObject::WriteProperty) {
        int flags = *reinterpret_cast<int*>(a[3]);
        if (!(flags & QDeclarativePropertyPrivate::BypassInterceptor)
            && !aInterceptors.isEmpty()
            && aInterceptors.testBit(id)) {
            QPair<int, QDeclarativePropertyValueInterceptor*> pair = interceptors.value(id);
            int valueIndex = pair.first;
            QDeclarativePropertyValueInterceptor *vi = pair.second;
            int type = property(id).userType();

            if (type != QVariant::Invalid) {
                if (valueIndex != -1) {
                    QDeclarativeEnginePrivate *ep = ctxt?QDeclarativeEnginePrivate::get(ctxt->engine):0;
                    QDeclarativeValueType *valueType = 0;
                    if (ep) valueType = ep->valueTypes[type];
                    else valueType = QDeclarativeValueTypeFactory::valueType(type);
                    Q_ASSERT(valueType);

                    valueType->setValue(QVariant(type, a[0]));
                    QMetaProperty valueProp = valueType->metaObject()->property(valueIndex);
                    vi->write(valueProp.read(valueType));

                    if (!ep) delete valueType;
                    return -1;
                } else {
                    vi->write(QVariant(type, a[0]));
                    return -1;
                }
            }
        }
    }
    if (c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty || c == QMetaObject::ResetProperty) {
        if (id >= propOffset) {
            id -= propOffset;

            if (id < metaData->propertyCount) {
               int t = (metaData->propertyData() + id)->propertyType;
                bool needActivate = false;

                if (t == -1) {

                    if (id >= firstVarPropertyIndex) {
                        // the context can be null if accessing var properties from cpp after re-parenting an item.
                        QDeclarativeEnginePrivate *ep = (ctxt == 0 || ctxt->engine == 0) ? 0 : QDeclarativeEnginePrivate::get(ctxt->engine);
                        QV8Engine *v8e = (ep == 0) ? 0 : ep->v8engine();
                        if (v8e) {
                            v8::HandleScope handleScope;
                            v8::Context::Scope contextScope(v8e->context());
                            if (c == QMetaObject::ReadProperty) {
                                *reinterpret_cast<QVariant *>(a[0]) = readPropertyAsVariant(id);
                            } else if (c == QMetaObject::WriteProperty) {
                                writeProperty(id, *reinterpret_cast<QVariant *>(a[0]));
                            }
                        } else if (c == QMetaObject::ReadProperty) {
                            // if the context was disposed, we just return an invalid variant from read.
                            *reinterpret_cast<QVariant *>(a[0]) = QVariant();
                        }
                    } else {
                        // don't need to set up v8 scope objects, since not accessing varProperties.
                        if (c == QMetaObject::ReadProperty) {
                            *reinterpret_cast<QVariant *>(a[0]) = readPropertyAsVariant(id);
                        } else if (c == QMetaObject::WriteProperty) {
                            writeProperty(id, *reinterpret_cast<QVariant *>(a[0]));
                        }
                    }

                } else {

                    if (c == QMetaObject::ReadProperty) {
                        switch(t) {
                        case QVariant::Int:
                            *reinterpret_cast<int *>(a[0]) = data[id].asInt();
                            break;
                        case QVariant::Bool:
                            *reinterpret_cast<bool *>(a[0]) = data[id].asBool();
                            break;
                        case QVariant::Double:
                            *reinterpret_cast<double *>(a[0]) = data[id].asDouble();
                            break;
                        case QVariant::String:
                            *reinterpret_cast<QString *>(a[0]) = data[id].asQString();
                            break;
                        case QVariant::Url:
                            *reinterpret_cast<QUrl *>(a[0]) = data[id].asQUrl();
                            break;
                        case QVariant::Color:
                            *reinterpret_cast<QColor *>(a[0]) = data[id].asQColor();
                            break;
                        case QVariant::Date:
                            *reinterpret_cast<QDate *>(a[0]) = data[id].asQDate();
                            break;
                        case QVariant::DateTime:
                            *reinterpret_cast<QDateTime *>(a[0]) = data[id].asQDateTime();
                            break;
                        case QMetaType::QObjectStar:
                            *reinterpret_cast<QObject **>(a[0]) = data[id].asQObject();
                            break;
                        default:
                            break;
                        }
                        if (t == qMetaTypeId<QDeclarativeListProperty<QObject> >()) {
                            int listIndex = data[id].asInt();
                            const List *list = &listProperties.at(listIndex);
                            *reinterpret_cast<QDeclarativeListProperty<QObject> *>(a[0]) = 
                                QDeclarativeListProperty<QObject>(object, (void *)list,
                                                                  list_append, list_count, list_at, 
                                                                  list_clear);
                        }

                    } else if (c == QMetaObject::WriteProperty) {

                        switch(t) {
                        case QVariant::Int:
                            needActivate = *reinterpret_cast<int *>(a[0]) != data[id].asInt();
                            data[id].setValue(*reinterpret_cast<int *>(a[0]));
                            break;
                        case QVariant::Bool:
                            needActivate = *reinterpret_cast<bool *>(a[0]) != data[id].asBool();
                            data[id].setValue(*reinterpret_cast<bool *>(a[0]));
                            break;
                        case QVariant::Double:
                            needActivate = *reinterpret_cast<double *>(a[0]) != data[id].asDouble();
                            data[id].setValue(*reinterpret_cast<double *>(a[0]));
                            break;
                        case QVariant::String:
                            needActivate = *reinterpret_cast<QString *>(a[0]) != data[id].asQString();
                            data[id].setValue(*reinterpret_cast<QString *>(a[0]));
                            break;
                        case QVariant::Url:
                            needActivate = *reinterpret_cast<QUrl *>(a[0]) != data[id].asQUrl();
                            data[id].setValue(*reinterpret_cast<QUrl *>(a[0]));
                            break;
                        case QVariant::Color:
                            needActivate = *reinterpret_cast<QColor *>(a[0]) != data[id].asQColor();
                            data[id].setValue(*reinterpret_cast<QColor *>(a[0]));
                            break;
                        case QVariant::Date:
                            needActivate = *reinterpret_cast<QDate *>(a[0]) != data[id].asQDate();
                            data[id].setValue(*reinterpret_cast<QDate *>(a[0]));
                            break;
                        case QVariant::DateTime:
                            needActivate = *reinterpret_cast<QDateTime *>(a[0]) != data[id].asQDateTime();
                            data[id].setValue(*reinterpret_cast<QDateTime *>(a[0]));
                            break;
                        case QMetaType::QObjectStar:
                            needActivate = *reinterpret_cast<QObject **>(a[0]) != data[id].asQObject();
                            data[id].setValue(*reinterpret_cast<QObject **>(a[0]));
                            break;
                        default:
                            break;
                        }
                    }

                }

                if (c == QMetaObject::WriteProperty && needActivate) {
                    activate(object, methodOffset + id, 0);
                }

                return -1;
            }

            id -= metaData->propertyCount;

            if (id < metaData->aliasCount) {

                QDeclarativeVMEMetaData::AliasData *d = metaData->aliasData() + id;

                if (d->flags & QML_ALIAS_FLAG_PTR && c == QMetaObject::ReadProperty) 
                        *reinterpret_cast<void **>(a[0]) = 0;

                if (!ctxt) return -1;

                QDeclarativeContext *context = ctxt->asQDeclarativeContext();
                QDeclarativeContextPrivate *ctxtPriv = QDeclarativeContextPrivate::get(context);

                QObject *target = ctxtPriv->data->idValues[d->contextIdx].data();
                if (!target) 
                    return -1;

                connectAlias(id);

                if (d->isObjectAlias()) {
                    *reinterpret_cast<QObject **>(a[0]) = target;
                    return -1;
                } 
                
                // Remove binding (if any) on write
                if(c == QMetaObject::WriteProperty) {
                    int flags = *reinterpret_cast<int*>(a[3]);
                    if (flags & QDeclarativePropertyPrivate::RemoveBindingOnAliasWrite) {
                        QDeclarativeData *targetData = QDeclarativeData::get(target);
                        if (targetData && targetData->hasBindingBit(d->propertyIndex())) {
                            QDeclarativeAbstractBinding *binding = QDeclarativePropertyPrivate::setBinding(target, d->propertyIndex(), d->isValueTypeAlias()?d->valueTypeIndex():-1, 0);
                            if (binding) binding->destroy();
                        }
                    }
                }
                
                if (d->isValueTypeAlias()) {
                    // Value type property
                    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(ctxt->engine);

                    QDeclarativeValueType *valueType = ep->valueTypes[d->valueType()];
                    Q_ASSERT(valueType);

                    valueType->read(target, d->propertyIndex());
                    int rv = QMetaObject::metacall(valueType, c, d->valueTypeIndex(), a);
                    
                    if (c == QMetaObject::WriteProperty)
                        valueType->write(target, d->propertyIndex(), 0x00);

                    return rv;

                } else {
                    return QMetaObject::metacall(target, c, d->propertyIndex(), a);
                }

            }
            return -1;

        }

    } else if(c == QMetaObject::InvokeMetaMethod) {

        if (id >= methodOffset) {

            id -= methodOffset;
            int plainSignals = metaData->signalCount + metaData->propertyCount +
                               metaData->aliasCount;
            if (id < plainSignals) {
                QMetaObject::activate(object, _id, a);
                return -1;
            }

            id -= plainSignals;

            if (id < metaData->methodCount) {
                if (!ctxt->engine)
                    return -1; // We can't run the method

                QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(ctxt->engine);
                ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.

                v8::Handle<v8::Function> function = method(id);
                QDeclarativeVMEMetaData::MethodData *data = metaData->methodData() + id;

                v8::HandleScope handle_scope;
                v8::Context::Scope scope(ep->v8engine()->context());
                v8::Handle<v8::Value> *args = 0;

                if (data->parameterCount) {
                    args = new v8::Handle<v8::Value>[data->parameterCount];
                    for (int ii = 0; ii < data->parameterCount; ++ii) 
                        args[ii] = ep->v8engine()->fromVariant(*(QVariant *)a[ii + 1]);
                }

                v8::TryCatch try_catch;

                v8::Local<v8::Value> result = function->Call(ep->v8engine()->global(), data->parameterCount, args);

                QVariant rv;
                if (try_catch.HasCaught()) {
                    QDeclarativeError error;
                    QDeclarativeExpressionPrivate::exceptionToError(try_catch.Message(), error);
                    if (error.isValid())
                        ep->warning(error);
                    if (a[0]) *(QVariant *)a[0] = QVariant();
                } else {
                    if (a[0]) *(QVariant *)a[0] = ep->v8engine()->toVariant(result, 0);
                }

                ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
                return -1;
            }
            return -1;
        }
    }

    if (parent)
        return parent->metaCall(c, _id, a);
    else
        return object->qt_metacall(c, _id, a);
}

v8::Handle<v8::Function> QDeclarativeVMEMetaObject::method(int index)
{
    if (!v8methods) 
        v8methods = new v8::Persistent<v8::Function>[metaData->methodCount];

    if (v8methods[index].IsEmpty()) {
        QDeclarativeVMEMetaData::MethodData *data = metaData->methodData() + index;

        const QChar *body = 
            (const QChar *)(((const char*)metaData) + data->bodyOffset);

        QString code = QString::fromRawData(body, data->bodyLength);

        // XXX We should evaluate all methods in a single big script block to 
        // improve the call time between dynamic methods defined on the same
        // object
        v8methods[index] = QDeclarativeExpressionPrivate::evalFunction(ctxt, object, code, ctxt->url.toString(),
                                                                       data->lineNumber);
    }

    return v8methods[index];
}

v8::Handle<v8::Value> QDeclarativeVMEMetaObject::readVarProperty(int id)
{
    Q_ASSERT(id >= firstVarPropertyIndex);

    ensureVarPropertiesAllocated();
    return varProperties->Get(id - firstVarPropertyIndex);
}

QVariant QDeclarativeVMEMetaObject::readPropertyAsVariant(int id)
{
    if (id >= firstVarPropertyIndex) {
        ensureVarPropertiesAllocated();
        return QDeclarativeEnginePrivate::get(ctxt->engine)->v8engine()->toVariant(varProperties->Get(id - firstVarPropertyIndex), -1);
    } else {
        if (data[id].dataType() == QMetaType::QObjectStar) {
            return QVariant::fromValue(data[id].asQObject());
        } else {
            return data[id].asQVariant();
        }
    }
}

void QDeclarativeVMEMetaObject::writeVarProperty(int id, v8::Handle<v8::Value> value)
{
    Q_ASSERT(id >= firstVarPropertyIndex);
    ensureVarPropertiesAllocated();

    // Importantly, if the current value is a scarce resource, we need to ensure that it
    // gets automatically released by the engine if no other references to it exist.
    v8::Local<v8::Value> oldv = varProperties->Get(id - firstVarPropertyIndex);
    if (oldv->IsObject()) {
        QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(oldv));
        if (r) {
            r->removeVmePropertyReference();
        }
    }

    // And, if the new value is a scarce resource, we need to ensure that it does not get
    // automatically released by the engine until no other references to it exist.
    if (value->IsObject()) {
        QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(value));
        if (r) {
            r->addVmePropertyReference();
        }
    }

    // Write the value and emit change signal as appropriate.
    varProperties->Set(id - firstVarPropertyIndex, value);
    activate(object, methodOffset + id, 0);
}

void QDeclarativeVMEMetaObject::writeProperty(int id, const QVariant &value)
{
    if (id >= firstVarPropertyIndex) {
        ensureVarPropertiesAllocated();

        // Importantly, if the current value is a scarce resource, we need to ensure that it
        // gets automatically released by the engine if no other references to it exist.
        v8::Local<v8::Value> oldv = varProperties->Get(id - firstVarPropertyIndex);
        if (oldv->IsObject()) {
            QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(oldv));
            if (r) {
                r->removeVmePropertyReference();
            }
        }

        // And, if the new value is a scarce resource, we need to ensure that it does not get
        // automatically released by the engine until no other references to it exist.
        v8::Handle<v8::Value> newv = QDeclarativeEnginePrivate::get(ctxt->engine)->v8engine()->fromVariant(value);
        if (newv->IsObject()) {
            QV8VariantResource *r = v8_resource_cast<QV8VariantResource>(v8::Handle<v8::Object>::Cast(newv));
            if (r) {
                r->addVmePropertyReference();
            }
        }

        // Write the value and emit change signal as appropriate.
        QVariant currentValue = readPropertyAsVariant(id);
        varProperties->Set(id - firstVarPropertyIndex, newv);
        if ((currentValue.userType() != value.userType() || currentValue != value))
            activate(object, methodOffset + id, 0);
    } else {
        bool needActivate = false;
        if (value.userType() == QMetaType::QObjectStar) {
            QObject *o = qvariant_cast<QObject *>(value);
            needActivate = (data[id].dataType() != QMetaType::QObjectStar || data[id].asQObject() != o);
            data[id].setValue(qvariant_cast<QObject *>(value));
        } else {
            needActivate = (data[id].dataType() != qMetaTypeId<QVariant>() ||
                            data[id].asQVariant().userType() != value.userType() ||
                            data[id].asQVariant() != value);
            data[id].setValue(value);
        }

        if (needActivate)
            activate(object, methodOffset + id, 0);
    }
}

void QDeclarativeVMEMetaObject::listChanged(int id)
{
    activate(object, methodOffset + id, 0);
}

void QDeclarativeVMEMetaObject::list_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    List *list = static_cast<List *>(prop->data);
    list->append(o);
    QMetaObject::activate(prop->object, list->notifyIndex, 0);
}

int QDeclarativeVMEMetaObject::list_count(QDeclarativeListProperty<QObject> *prop)
{
    return static_cast<List *>(prop->data)->count();
}

QObject *QDeclarativeVMEMetaObject::list_at(QDeclarativeListProperty<QObject> *prop, int index)
{
    return static_cast<List *>(prop->data)->at(index);
}

void QDeclarativeVMEMetaObject::list_clear(QDeclarativeListProperty<QObject> *prop)
{
    List *list = static_cast<List *>(prop->data);
    list->clear();
    QMetaObject::activate(prop->object, list->notifyIndex, 0);
}

void QDeclarativeVMEMetaObject::registerInterceptor(int index, int valueIndex, QDeclarativePropertyValueInterceptor *interceptor)
{
    if (aInterceptors.isEmpty())
        aInterceptors.resize(propertyCount() + metaData->propertyCount);
    aInterceptors.setBit(index);
    interceptors.insert(index, qMakePair(valueIndex, interceptor));
}

int QDeclarativeVMEMetaObject::vmeMethodLineNumber(int index)
{
    if (index < methodOffset) {
        Q_ASSERT(parent);
        return static_cast<QDeclarativeVMEMetaObject *>(parent)->vmeMethodLineNumber(index);
    }

    int plainSignals = metaData->signalCount + metaData->propertyCount + metaData->aliasCount;
    Q_ASSERT(index >= (methodOffset + plainSignals) && index < (methodOffset + plainSignals + metaData->methodCount));

    int rawIndex = index - methodOffset - plainSignals;

    QDeclarativeVMEMetaData::MethodData *data = metaData->methodData() + rawIndex;
    return data->lineNumber;
}

v8::Handle<v8::Function> QDeclarativeVMEMetaObject::vmeMethod(int index)
{
    if (index < methodOffset) {
        Q_ASSERT(parent);
        return static_cast<QDeclarativeVMEMetaObject *>(parent)->vmeMethod(index);
    }
    int plainSignals = metaData->signalCount + metaData->propertyCount + metaData->aliasCount;
    Q_ASSERT(index >= (methodOffset + plainSignals) && index < (methodOffset + plainSignals + metaData->methodCount));
    return method(index - methodOffset - plainSignals);
}

// Used by debugger
void QDeclarativeVMEMetaObject::setVmeMethod(int index, v8::Persistent<v8::Function> value)
{
    if (index < methodOffset) {
        Q_ASSERT(parent);
        return static_cast<QDeclarativeVMEMetaObject *>(parent)->setVmeMethod(index, value);
    }
    int plainSignals = metaData->signalCount + metaData->propertyCount + metaData->aliasCount;
    Q_ASSERT(index >= (methodOffset + plainSignals) && index < (methodOffset + plainSignals + metaData->methodCount));

    if (!v8methods) 
        v8methods = new v8::Persistent<v8::Function>[metaData->methodCount];

    int methodIndex = index - methodOffset - plainSignals;
    if (!v8methods[methodIndex].IsEmpty()) 
        qPersistentDispose(v8methods[methodIndex]);
    v8methods[methodIndex] = value;
}

v8::Handle<v8::Value> QDeclarativeVMEMetaObject::vmeProperty(int index)
{
    if (index < propOffset) {
        Q_ASSERT(parent);
        return static_cast<QDeclarativeVMEMetaObject *>(parent)->vmeProperty(index);
    }
    return readVarProperty(index - propOffset);
}

void QDeclarativeVMEMetaObject::setVMEProperty(int index, v8::Handle<v8::Value> v)
{
    if (index < propOffset) {
        Q_ASSERT(parent);
        static_cast<QDeclarativeVMEMetaObject *>(parent)->setVMEProperty(index, v);
    }
    return writeVarProperty(index - propOffset, v);
}

void QDeclarativeVMEMetaObject::ensureVarPropertiesAllocated()
{
    if (!varPropertiesInitialized)
        allocateVarPropertiesArray();
}

// see also: QV8GCCallback::garbageCollectorPrologueCallback()
void QDeclarativeVMEMetaObject::allocateVarPropertiesArray()
{
    v8::HandleScope handleScope;
    v8::Context::Scope cs(QDeclarativeEnginePrivate::get(ctxt->engine)->v8engine()->context());
    varProperties = qPersistentNew(v8::Array::New(metaData->varPropertyCount));
    varProperties.MakeWeak(static_cast<void*>(this), VarPropertiesWeakReferenceCallback);
    varPropertiesInitialized = true;
}

/*
   The "var" properties are stored in a v8::Array which will be strong persistent if the object has cpp-ownership
   and the root QObject in the parent chain does not have JS-ownership.  In the weak persistent handle case,
   this callback will dispose the handle when the v8object which owns the lifetime of the var properties array
   is cleared as a result of all other handles to that v8object being released.
   See QV8GCCallback::garbageCollectorPrologueCallback() for more information.
 */
void QDeclarativeVMEMetaObject::VarPropertiesWeakReferenceCallback(v8::Persistent<v8::Value> object, void* parameter)
{
    QDeclarativeVMEMetaObject *vmemo = static_cast<QDeclarativeVMEMetaObject*>(parameter);
    Q_ASSERT(vmemo);
    qPersistentDispose(object);
    vmemo->varProperties.Clear();
}

void QDeclarativeVMEMetaObject::GcPrologueCallback(QV8GCCallback::Referencer *r, QV8GCCallback::Node *node)
{
    QDeclarativeVMEMetaObject *vmemo = static_cast<QDeclarativeVMEMetaObject*>(node);
    Q_ASSERT(vmemo);
    if (!vmemo->varPropertiesInitialized || vmemo->varProperties.IsEmpty())
        return;
    r->addRelationship(vmemo->object, vmemo->varProperties);
}

bool QDeclarativeVMEMetaObject::aliasTarget(int index, QObject **target, int *coreIndex, int *valueTypeIndex) const
{
    Q_ASSERT(index >= propOffset + metaData->propertyCount);

    *target = 0;
    *coreIndex = -1;
    *valueTypeIndex = -1;

    if (!ctxt)
        return false;

    QDeclarativeVMEMetaData::AliasData *d = metaData->aliasData() + (index - propOffset - metaData->propertyCount);
    QDeclarativeContext *context = ctxt->asQDeclarativeContext();
    QDeclarativeContextPrivate *ctxtPriv = QDeclarativeContextPrivate::get(context);

    *target = ctxtPriv->data->idValues[d->contextIdx].data();
    if (!*target)
        return false;

    if (d->isObjectAlias()) {
    } else if (d->isValueTypeAlias()) {
        *coreIndex = d->propertyIndex();
        *valueTypeIndex = d->valueTypeIndex();
    } else {
        *coreIndex = d->propertyIndex();
    }

    return true;
}

void QDeclarativeVMEMetaObject::connectAlias(int aliasId)
{
    if (!aConnected.testBit(aliasId)) {

        if (!aliasEndpoints)
            aliasEndpoints = new QDeclarativeVMEMetaObjectEndpoint[metaData->aliasCount];

        aConnected.setBit(aliasId);

        QDeclarativeVMEMetaData::AliasData *d = metaData->aliasData() + aliasId;

        QDeclarativeVMEMetaObjectEndpoint *endpoint = aliasEndpoints + aliasId;
        endpoint->metaObject = this;

        endpoint->connect(&ctxt->idValues[d->contextIdx].bindings);

        endpoint->tryConnect();
    }
}

void QDeclarativeVMEMetaObject::connectAliasSignal(int index)
{
    int aliasId = (index - methodOffset) - metaData->propertyCount;
    if (aliasId < 0 || aliasId >= metaData->aliasCount)
        return;

    connectAlias(aliasId);
}

QT_END_NAMESPACE
