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

#include "qv8engine_p.h"

#include "qv4sequenceobject_p.h"
#include "qjsengine_p.h"

#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmllist_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlxmlhttprequest_p.h>
#include <private/qqmllocale_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlmemoryprofiler_p.h>
#include <private/qqmlplatform_p.h>
#include <private/qjsvalue_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qqmllistwrapper_p.h>

#include "qv4domerrors_p.h"
#include "qv4sqlerrors_p.h"

#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qdatetime.h>

#include <private/qv4value_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4globalobject_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4variantobject_p.h>
#include <private/qv4script_p.h>
#include <private/qv4include_p.h>

Q_DECLARE_METATYPE(QList<int>)


// XXX TODO: Need to check all the global functions will also work in a worker script where the
// QQmlEngine is not available
QT_BEGIN_NAMESPACE


QV8Engine::QV8Engine(QJSEngine* qq)
    : q(qq)
    , m_engine(0)
    , m_xmlHttpRequestData(0)
    , m_listModelData(0)
{
    QML_MEMORY_SCOPE_STRING("QV8Engine::QV8Engine");
    qMetaTypeId<QJSValue>();
    qMetaTypeId<QList<int> >();

    ensurePerThreadIsolate();

    m_v4Engine = new QV4::ExecutionEngine;
    v8::Isolate::SetEngine(m_v4Engine);
    m_v4Engine->publicEngine = q;

    QV8GCCallback::registerGcPrologueCallback();
    m_strongReferencer = QV4::Value::fromObject(m_v4Engine->newObject());

    m_bindingFlagKey = QV4::Value::fromString(m_v4Engine->current, QStringLiteral("qml::binding"));

    m_qobjectWrapper.init(this);
    m_jsonWrapper.init(m_v4Engine);

}

QV8Engine::~QV8Engine()
{
    for (int ii = 0; ii < m_extensionData.count(); ++ii)
        delete m_extensionData[ii];
    m_extensionData.clear();

    qt_rem_qmlxmlhttprequest(this, m_xmlHttpRequestData);
    m_xmlHttpRequestData = 0;
    delete m_listModelData;
    m_listModelData = 0;

    m_jsonWrapper.destroy();
    m_qobjectWrapper.destroy();

    v8::Isolate::SetEngine(0);
    delete m_v4Engine;
}

QVariant QV8Engine::toVariant(const QV4::Value &value, int typeHint)
{
    if (value.isEmpty())
        return QVariant();

    if (QV4::VariantObject *v = value.asVariantObject())
        return v->data;

    if (typeHint == QVariant::Bool)
        return QVariant(value.toBoolean());

    if (typeHint == QMetaType::QJsonValue)
        return QVariant::fromValue(jsonValueFromJS(value));

    if (typeHint == qMetaTypeId<QJSValue>())
        return QVariant::fromValue(QJSValue(new QJSValuePrivate(m_v4Engine, value)));

    if (QV4::Object *object = value.asObject()) {
        QV8ObjectResource *r = (QV8ObjectResource *)v8::Handle<v8::Value>(value)->ToObject()->GetExternalResource();
        if (r) {
            switch (r->resourceType()) {
            case QV8ObjectResource::Context2DStyleType:
            case QV8ObjectResource::Context2DPixelArrayType:
            case QV8ObjectResource::SignalHandlerType:
            case QV8ObjectResource::IncubatorType:
            case QV8ObjectResource::VisualDataItemType:
            case QV8ObjectResource::XMLHttpRequestType:
            case QV8ObjectResource::DOMNodeType:
            case QV8ObjectResource::SQLDatabaseType:
            case QV8ObjectResource::ListModelType:
            case QV8ObjectResource::ParticleDataType:
            case QV8ObjectResource::ChangeSetArrayType:
                return QVariant();
            }
        } else if (typeHint == QMetaType::QJsonObject
                   && !value.asArrayObject() && !value.asFunctionObject()) {
            return QVariant::fromValue(jsonObjectFromJS(value));
        } else if (QV4::QObjectWrapper *wrapper = object->asQObjectWrapper()) {
            return qVariantFromValue<QObject *>(wrapper->object);
        } else if (QV4::QmlContextWrapper *wrapper = object->asQmlContext()) {
            return QVariant();
        } else if (QV4::QmlTypeWrapper *w = object->asQmlTypeWrapper()) {
            return w->toVariant();
        } else if (QV4::QmlValueTypeWrapper *v = object->asQmlValueTypeWrapper()) {
            return v->toVariant();
        } else if (QV4::QmlListWrapper *l = object->asQmlListWrapper()) {
            return l->toVariant();
        } else if (object->isListType())
            return QV4::SequencePrototype::toVariant(object);
    }

    if (QV4::ArrayObject *a = value.asArrayObject()) {
        if (typeHint == qMetaTypeId<QList<QObject *> >()) {
            QList<QObject *> list;
            uint32_t length = a->arrayLength();
            for (uint32_t ii = 0; ii < length; ++ii) {
                QV4::Value arrayItem = a->getIndexed(m_v4Engine->current, ii);
                if (arrayItem.isObject()) {
                    list << toQObject(arrayItem);
                } else {
                    list << 0;
                }
            }

            return qVariantFromValue<QList<QObject*> >(list);
        } else if (typeHint == QMetaType::QJsonArray) {
            return QVariant::fromValue(jsonArrayFromJS(value));
        }

        bool succeeded = false;
        QVariant retn = QV4::SequencePrototype::toVariant(value, typeHint, &succeeded);
        if (succeeded)
            return retn;
    }

    return toBasicVariant(value);
}

static QV4::Value arrayFromStringList(QV8Engine *engine, const QStringList &list)
{
    QV4::ExecutionEngine *e = QV8Engine::getV4(engine);
    QV4::ArrayObject *a = e->newArrayObject();
    int len = list.count();
    a->arrayReserve(len);
    for (int ii = 0; ii < len; ++ii)
        a->arrayData[ii].value = QV4::Value::fromString(e->newString(list.at(ii)));
    a->arrayDataLen = len;
    a->setArrayLengthUnchecked(len);
    return QV4::Value::fromObject(a);
}

static QV4::Value arrayFromVariantList(QV8Engine *engine, const QVariantList &list)
{
    QV4::ExecutionEngine *e = QV8Engine::getV4(engine);
    QV4::ArrayObject *a = e->newArrayObject();
    int len = list.count();
    a->arrayReserve(len);
    for (int ii = 0; ii < len; ++ii)
        a->arrayData[ii].value = engine->fromVariant(list.at(ii));
    a->arrayDataLen = len;
    a->setArrayLengthUnchecked(len);
    return QV4::Value::fromObject(a);
}

static QV4::Value objectFromVariantMap(QV8Engine *engine, const QVariantMap &map)
{
    QV4::ExecutionEngine *e = QV8Engine::getV4(engine);
    QV4::Object *o = e->newObject();
    for (QVariantMap::ConstIterator iter = map.begin(); iter != map.end(); ++iter)
        o->put(e->current, e->newString(iter.key()), engine->fromVariant(iter.value()));
    return QV4::Value::fromObject(o);
}

Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &, QRegExp::PatternSyntax);

QV4::Value QV8Engine::fromVariant(const QVariant &variant)
{
    int type = variant.userType();
    const void *ptr = variant.constData();

    if (type < QMetaType::User) {
        switch (QMetaType::Type(type)) {
            case QMetaType::UnknownType:
            case QMetaType::Void:
                return QV4::Value::undefinedValue();
            case QMetaType::Bool:
                return QV4::Value::fromBoolean(*reinterpret_cast<const bool*>(ptr));
            case QMetaType::Int:
                return QV4::Value::fromInt32(*reinterpret_cast<const int*>(ptr));
            case QMetaType::UInt:
                return QV4::Value::fromUInt32(*reinterpret_cast<const uint*>(ptr));
            case QMetaType::LongLong:
                return QV4::Value::fromDouble(*reinterpret_cast<const qlonglong*>(ptr));
            case QMetaType::ULongLong:
                return QV4::Value::fromDouble(*reinterpret_cast<const qulonglong*>(ptr));
            case QMetaType::Double:
                return QV4::Value::fromDouble(*reinterpret_cast<const double*>(ptr));
            case QMetaType::QString:
                return QV4::Value::fromString(m_v4Engine->current, *reinterpret_cast<const QString*>(ptr));
            case QMetaType::Float:
                return QV4::Value::fromDouble(*reinterpret_cast<const float*>(ptr));
            case QMetaType::Short:
                return QV4::Value::fromInt32(*reinterpret_cast<const short*>(ptr));
            case QMetaType::UShort:
                return QV4::Value::fromUInt32(*reinterpret_cast<const unsigned short*>(ptr));
            case QMetaType::Char:
                return QV4::Value::fromInt32(*reinterpret_cast<const char*>(ptr));
            case QMetaType::UChar:
                return QV4::Value::fromUInt32(*reinterpret_cast<const unsigned char*>(ptr));
            case QMetaType::QChar:
                return QV4::Value::fromInt32((*reinterpret_cast<const QChar*>(ptr)).unicode());
            case QMetaType::QDateTime:
                return QV4::Value::fromObject(m_v4Engine->newDateObject(*reinterpret_cast<const QDateTime *>(ptr)));
            case QMetaType::QDate:
                return QV4::Value::fromObject(m_v4Engine->newDateObject(QDateTime(*reinterpret_cast<const QDate *>(ptr))));
            case QMetaType::QTime:
            return QV4::Value::fromObject(m_v4Engine->newDateObject(QDateTime(QDate(1970,1,1), *reinterpret_cast<const QTime *>(ptr))));
            case QMetaType::QRegExp:
                return QV4::Value::fromObject(m_v4Engine->newRegExpObject(*reinterpret_cast<const QRegExp *>(ptr)));
            case QMetaType::QObjectStar:
                return newQObject(*reinterpret_cast<QObject* const *>(ptr));
            case QMetaType::QStringList:
                {
                bool succeeded = false;
                v8::Handle<v8::Value> retn = QV4::SequencePrototype::fromVariant(m_v4Engine, variant, &succeeded);
                if (succeeded)
                    return retn->v4Value();
                return arrayFromStringList(this, *reinterpret_cast<const QStringList *>(ptr));
                }
            case QMetaType::QVariantList:
                return arrayFromVariantList(this, *reinterpret_cast<const QVariantList *>(ptr));
            case QMetaType::QVariantMap:
                return objectFromVariantMap(this, *reinterpret_cast<const QVariantMap *>(ptr));
            case QMetaType::QJsonValue:
                return jsonValueToJS(*reinterpret_cast<const QJsonValue *>(ptr));
            case QMetaType::QJsonObject:
                return jsonObjectToJS(*reinterpret_cast<const QJsonObject *>(ptr));
            case QMetaType::QJsonArray:
                return jsonArrayToJS(*reinterpret_cast<const QJsonArray *>(ptr));

            default:
                break;
        }

        if (QQmlValueType *vt = QQmlValueTypeFactory::valueType(type))
            return QV4::QmlValueTypeWrapper::create(this, variant, vt);
    } else {
        if (type == qMetaTypeId<QQmlListReference>()) {
            typedef QQmlListReferencePrivate QDLRP;
            QDLRP *p = QDLRP::get((QQmlListReference*)ptr);
            if (p->object) {
                return QV4::QmlListWrapper::create(this, p->property, p->propertyType);
            } else {
                return QV4::Value::nullValue();
            }
        } else if (type == qMetaTypeId<QJSValue>()) {
            const QJSValue *value = reinterpret_cast<const QJSValue *>(ptr);
            QJSValuePrivate *valuep = QJSValuePrivate::get(*value);
            return valuep->getValue(m_v4Engine);
        } else if (type == qMetaTypeId<QList<QObject *> >()) {
            // XXX Can this be made more by using Array as a prototype and implementing
            // directly against QList<QObject*>?
            const QList<QObject *> &list = *(QList<QObject *>*)ptr;
            QV4::ArrayObject *a = m_v4Engine->newArrayObject();
            a->setArrayLength(list.count());
            for (int ii = 0; ii < list.count(); ++ii)
                a->arrayData[ii].value = newQObject(list.at(ii));
            return QV4::Value::fromObject(a);
        }

        bool objOk;
        QObject *obj = QQmlMetaType::toQObject(variant, &objOk);
        if (objOk)
            return newQObject(obj);

        bool succeeded = false;
        v8::Handle<v8::Value> retn = QV4::SequencePrototype::fromVariant(m_v4Engine, variant, &succeeded);
        if (succeeded)
            return retn->v4Value();

        if (QQmlValueType *vt = QQmlValueTypeFactory::valueType(type))
            return QV4::QmlValueTypeWrapper::create(this, variant, vt);
    }

    // XXX TODO: To be compatible, we still need to handle:
    //    + QObjectList
    //    + QList<int>

    return QV4::Value::fromObject(m_v4Engine->newVariantObject(variant));
}

QNetworkAccessManager *QV8Engine::networkAccessManager()
{
    return QQmlEnginePrivate::get(m_engine)->getNetworkAccessManager();
}

const QStringHash<bool> &QV8Engine::illegalNames() const
{
    return m_illegalNames;
}

// Requires a handle scope
QV4::Value QV8Engine::getOwnPropertyNames(const QV4::Value &o)
{
    if (!o.asObject())
        return QV4::Value::fromObject(m_v4Engine->newArrayObject());
    QV4::SimpleCallContext ctx;
    ctx.initSimpleCallContext(m_v4Engine);
    QV4::Value args = o;
    ctx.arguments = &args;
    ctx.argumentCount = 1;
    m_v4Engine->pushContext(&ctx);
    QV4::Value result = QV4::ObjectPrototype::method_getOwnPropertyNames(&ctx);
    m_v4Engine->popContext();
    return result;
}

QQmlContextData *QV8Engine::callingContext()
{
    return QV4::QmlContextWrapper::callingContext(m_v4Engine);
}

// Converts a JS value to a QVariant.
// Null, Undefined -> QVariant() (invalid)
// Boolean -> QVariant(bool)
// Number -> QVariant(double)
// String -> QVariant(QString)
// Array -> QVariantList(...)
// Date -> QVariant(QDateTime)
// RegExp -> QVariant(QRegExp)
// [Any other object] -> QVariantMap(...)
QVariant QV8Engine::toBasicVariant(const QV4::Value &value)
{
    if (value.isNull() || value.isUndefined())
        return QVariant();
    if (value.isBoolean())
        return value.booleanValue();
    if (value.isInteger())
        return value.integerValue();
    if (value.isNumber())
        return value.asDouble();
    if (value.isString())
        return value.stringValue()->toQString();
    if (QV4::DateObject *d = value.asDateObject())
        return d->toQDateTime();
    // NOTE: since we convert QTime to JS Date, round trip will change the variant type (to QDateTime)!

    Q_ASSERT(value.isObject());

    if (QV4::RegExpObject *re = value.asRegExpObject())
        return re->toQRegExp();
    if (QV4::ArrayObject *a = value.asArrayObject()) {
        QVariantList rv;

        int length = a->arrayLength();
        for (int ii = 0; ii < length; ++ii)
            rv << toVariant(a->getIndexed(m_v4Engine->current, ii), -1);
        return rv;
    }
    if (!value.asFunctionObject())
        return variantMapFromJS(value.asObject());

    return QVariant();
}



void QV8Engine::initializeGlobal(v8::Handle<v8::Object> global)
{
    QV4::GlobalExtensions::init(m_engine, m_v4Engine->globalObject);

    QQmlLocale::registerStringLocaleCompare(m_v4Engine);
    QQmlDateExtension::registerExtension(m_v4Engine);
    QQmlNumberExtension::registerExtension(m_v4Engine);

    qt_add_domexceptions(m_v4Engine);
    m_xmlHttpRequestData = qt_add_qmlxmlhttprequest(this);

    qt_add_sqlexceptions(m_v4Engine);

    {
        for (uint i = 0; i < m_v4Engine->globalObject->internalClass->size; ++i)
            m_illegalNames.insert(m_v4Engine->globalObject->internalClass->nameMap.at(i)->toQString(), true);
    }

    {
#define FREEZE_SOURCE "(function freeze_recur(obj) { "\
                      "    if (Qt.isQtObject(obj)) return;"\
                      "    if (obj != Function.connect && obj != Function.disconnect && "\
                      "        obj instanceof Object) {"\
                      "        var properties = Object.getOwnPropertyNames(obj);"\
                      "        for (var prop in properties) { "\
                      "            if (prop == \"connect\" || prop == \"disconnect\") {"\
                      "                Object.freeze(obj[prop]); "\
                      "                continue;"\
                      "            }"\
                      "            freeze_recur(obj[prop]);"\
                      "        }"\
                      "    }"\
                      "    if (obj instanceof Object) {"\
                      "        Object.freeze(obj);"\
                      "    }"\
                      "})"

        QV4::Value result = QV4::Script::evaluate(m_v4Engine, QString::fromUtf8(FREEZE_SOURCE), 0);
        Q_ASSERT(result.asFunctionObject());
        m_freezeObject = result;
#undef FREEZE_SOURCE
    }
}

void QV8Engine::freezeObject(const QV4::Value &value)
{
    QV4::Value args = value;
    m_freezeObject.value().asFunctionObject()->call(m_v4Engine->rootContext, QV4::Value::fromObject(m_v4Engine->globalObject), &args, 1);
}

void QV8Engine::gc()
{
    m_v4Engine->memoryManager->runGC();
}

#ifdef QML_GLOBAL_HANDLE_DEBUGGING
#include <QtCore/qthreadstorage.h>
static QThreadStorage<QSet<void *> *> QV8Engine_activeHandles;

void QV8Engine::registerHandle(void *handle)
{
    if (!handle) {
        qWarning("Attempting to register a null handle");
        return;
    }

    if (!QV8Engine_activeHandles.hasLocalData())
        QV8Engine_activeHandles.setLocalData(new QSet<void *>);

    if (QV8Engine_activeHandles.localData()->contains(handle)) {
        qFatal("Handle %p already alive", handle);
    } else {
        QV8Engine_activeHandles.localData()->insert(handle);
    }
}

void QV8Engine::releaseHandle(void *handle)
{
    if (!handle)
        return;

    if (!QV8Engine_activeHandles.hasLocalData())
        QV8Engine_activeHandles.setLocalData(new QSet<void *>);

    if (QV8Engine_activeHandles.localData()->contains(handle)) {
        QV8Engine_activeHandles.localData()->remove(handle);
    } else {
        qFatal("Handle %p already dead", handle);
    }
}
#endif

struct QV8EngineRegistrationData
{
    QV8EngineRegistrationData() : extensionCount(0) {}

    QMutex mutex;
    int extensionCount;
};
Q_GLOBAL_STATIC(QV8EngineRegistrationData, registrationData);

QMutex *QV8Engine::registrationMutex()
{
    return &registrationData()->mutex;
}

int QV8Engine::registerExtension()
{
    return registrationData()->extensionCount++;
}

void QV8Engine::setExtensionData(int index, Deletable *data)
{
    if (m_extensionData.count() <= index)
        m_extensionData.resize(index + 1);

    if (m_extensionData.at(index))
        delete m_extensionData.at(index);

    m_extensionData[index] = data;
}


QV4::WeakValue *QV8Engine::findOwnerAndStrength(QObject *object, bool *shouldBeStrong)
{
    QQmlData *data = QQmlData::get(object);
    if (data && data->rootObjectInCreation) { // When the object is still being created it may not show up to the GC.
        *shouldBeStrong = true;
        return 0;
    }

    QObject *parent = object->parent();
    if (!parent) {
        // if the object has JS ownership, the object's v8object owns the lifetime of the persistent value.
        if (QQmlEngine::objectOwnership(object) == QQmlEngine::JavaScriptOwnership) {
            *shouldBeStrong = false;
            return &(QQmlData::get(object)->v8object);
        }

        // no parent, and has CPP ownership - doesn't have an implicit parent.
        *shouldBeStrong = true;
        return 0;
    }

    // if it is owned by CPP, it's root parent may still be owned by JS.
    // in that case, the owner of the persistent handle is the root parent's v8object.
    while (parent->parent())
        parent = parent->parent();

    if (QQmlEngine::objectOwnership(parent) == QQmlEngine::JavaScriptOwnership) {
        // root parent is owned by JS.  It's v8object owns the persistent value in question.
        *shouldBeStrong = false;
        return &(QQmlData::get(parent)->v8object);
    } else {
        // root parent has CPP ownership.  The persistent value should not be made weak.
        *shouldBeStrong = true;
        return 0;
    }
}

void QV8Engine::addRelationshipForGC(QObject *object, const QV4::PersistentValue &handle)
{
    if (!object || handle.isEmpty())
        return;

    bool handleShouldBeStrong = false;
    QV4::WeakValue *implicitOwner = findOwnerAndStrength(object, &handleShouldBeStrong);
    if (handleShouldBeStrong) {
        // ### FIXME
//        v8::V8::AddImplicitReferences(m_strongReferencer, &handle, 1);
    } else if (!implicitOwner->isEmpty()) {
        // ### FIXME
        qWarning() << "Fix object ownership";
//        v8::V8::AddImplicitReferences(*implicitOwner, &handle, 1);
    }
}

void QV8Engine::addRelationshipForGC(QObject *object, QObject *other)
{
    if (!object || !other)
        return;

    bool handleShouldBeStrong = false;
    QV4::WeakValue *implicitOwner = findOwnerAndStrength(object, &handleShouldBeStrong);
    QV4::WeakValue handle = QQmlData::get(other, true)->v8object;
    if (handle.isEmpty()) // no JS data to keep alive.
        return;
    // ### FIXME
    qWarning() << "Fix object ownership";
//    else if (handleShouldBeStrong)
//        v8::V8::AddImplicitReferences(m_strongReferencer, &handle, 1);
//    else if (!implicitOwner->IsEmpty())
//        v8::V8::AddImplicitReferences(*implicitOwner, &handle, 1);
}

static QThreadStorage<QV8Engine::ThreadData*> perThreadEngineData;

bool QV8Engine::hasThreadData()
{
    return perThreadEngineData.hasLocalData();
}

QV8Engine::ThreadData *QV8Engine::threadData()
{
    Q_ASSERT(perThreadEngineData.hasLocalData());
    return perThreadEngineData.localData();
}

void QV8Engine::ensurePerThreadIsolate()
{
    if (!perThreadEngineData.hasLocalData())
        perThreadEngineData.setLocalData(new ThreadData);
}

void QV8Engine::initQmlGlobalObject()
{
    initializeGlobal(QV4::Value::fromObject(m_v4Engine->globalObject));
    freezeObject(QV4::Value::fromObject(m_v4Engine->globalObject));
}

void QV8Engine::setEngine(QQmlEngine *engine)
{
    m_engine = engine;
    initQmlGlobalObject();
}

QV4::Value QV8Engine::global()
{
    return QV4::Value::fromObject(m_v4Engine->globalObject);
}

// Converts a QVariantList to JS.
// The result is a new Array object with length equal to the length
// of the QVariantList, and the elements being the QVariantList's
// elements converted to JS, recursively.
QV4::Value QV8Engine::variantListToJS(const QVariantList &lst)
{
    QV4::ArrayObject *a = m_v4Engine->newArrayObject();
    a->arrayReserve(lst.size());
    for (int i = 0; i < lst.size(); i++)
        a->arrayData[i].value = variantToJS(lst.at(i));
    a->setArrayLengthUnchecked(lst.size());
    return QV4::Value::fromObject(a);
}

// Converts a JS Array object to a QVariantList.
// The result is a QVariantList with length equal to the length
// of the JS Array, and elements being the JS Array's elements
// converted to QVariants, recursively.
QVariantList QV8Engine::variantListFromJS(QV4::ArrayObject *a,
                                          V8ObjectSet &visitedObjects)
{
    QVariantList result;
    if (!a)
        return result;

    if (visitedObjects.contains(a))
        // Avoid recursion.
        return result;

    visitedObjects.insert(a);

    quint32 length = a->arrayLength();
    for (quint32 i = 0; i < length; ++i) {
        QV4::Value v = a->getIndexed(m_v4Engine->current, i);
        result.append(variantFromJS(v, visitedObjects));
    }

    visitedObjects.remove(a);

    return result;
}

// Converts a QVariantMap to JS.
// The result is a new Object object with property names being
// the keys of the QVariantMap, and values being the values of
// the QVariantMap converted to JS, recursively.
QV4::Value QV8Engine::variantMapToJS(const QVariantMap &vmap)
{
    QV4::Object *o = m_v4Engine->newObject();
    QVariantMap::const_iterator it;
    for (it = vmap.constBegin(); it != vmap.constEnd(); ++it) {
        QV4::Property *p = o->insertMember(m_v4Engine->newIdentifier(it.key()), QV4::Attr_Data);
        p->value = variantToJS(it.value());
    }
    return QV4::Value::fromObject(o);
}

// Converts a JS Object to a QVariantMap.
// The result is a QVariantMap with keys being the property names
// of the object, and values being the values of the JS object's
// properties converted to QVariants, recursively.
QVariantMap QV8Engine::variantMapFromJS(QV4::Object *o,
                                        V8ObjectSet &visitedObjects)
{
    QVariantMap result;

    if (!o || o->asFunctionObject())
        return result;

    if (visitedObjects.contains(o)) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty object (and no error is thrown).
        return result;
    }

    visitedObjects.insert(o);

    QV4::ObjectIterator it(o, QV4::ObjectIterator::EnumerableOnly);
    while (1) {
        QV4::PropertyAttributes attributes;
        QV4::String *name;
        uint idx;
        QV4::Property *p = it.next(&name, &idx, &attributes);
        if (!p)
            break;

        QV4::Value v = o->getValue(m_v4Engine->current, p, attributes);
        QString key = name ? name->toQString() : QString::number(idx);
        result.insert(key, variantFromJS(v, visitedObjects));
    }

    visitedObjects.remove(o);
    return result;
}

// Converts the meta-type defined by the given type and data to JS.
// Returns the value if conversion succeeded, an empty handle otherwise.
QV4::Value QV8Engine::metaTypeToJS(int type, const void *data)
{
    Q_ASSERT(data != 0);
    QV4::Value result;

    // check if it's one of the types we know
    switch (QMetaType::Type(type)) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
        return QV4::Value::undefinedValue();
    case QMetaType::Bool:
        return QV4::Value::fromBoolean(*reinterpret_cast<const bool*>(data));
    case QMetaType::Int:
        return QV4::Value::fromInt32(*reinterpret_cast<const int*>(data));
    case QMetaType::UInt:
        return QV4::Value::fromUInt32(*reinterpret_cast<const uint*>(data));
    case QMetaType::LongLong:
        return QV4::Value::fromDouble(double(*reinterpret_cast<const qlonglong*>(data)));
    case QMetaType::ULongLong:
#if defined(Q_OS_WIN) && defined(_MSC_FULL_VER) && _MSC_FULL_VER <= 12008804
#pragma message("** NOTE: You need the Visual Studio Processor Pack to compile support for 64bit unsigned integers.")
        return QV4::Value::fromDouble(double((qlonglong)*reinterpret_cast<const qulonglong*>(data)));
#elif defined(Q_CC_MSVC) && !defined(Q_CC_MSVC_NET)
        return QV4::Value::fromDouble(double((qlonglong)*reinterpret_cast<const qulonglong*>(data)));
#else
        return QV4::Value::fromDouble(double(*reinterpret_cast<const qulonglong*>(data)));
#endif
    case QMetaType::Double:
        return QV4::Value::fromDouble(*reinterpret_cast<const double*>(data));
    case QMetaType::QString:
        return QV4::Value::fromString(m_v4Engine->current, *reinterpret_cast<const QString*>(data));
    case QMetaType::Float:
        return QV4::Value::fromDouble(*reinterpret_cast<const float*>(data));
    case QMetaType::Short:
        return QV4::Value::fromInt32(*reinterpret_cast<const short*>(data));
    case QMetaType::UShort:
        return QV4::Value::fromUInt32(*reinterpret_cast<const unsigned short*>(data));
    case QMetaType::Char:
        return QV4::Value::fromInt32(*reinterpret_cast<const char*>(data));
    case QMetaType::UChar:
        return QV4::Value::fromUInt32(*reinterpret_cast<const unsigned char*>(data));
    case QMetaType::QChar:
        return QV4::Value::fromUInt32((*reinterpret_cast<const QChar*>(data)).unicode());
    case QMetaType::QStringList:
        result = QV4::Value::fromObject(m_v4Engine->newArrayObject(*reinterpret_cast<const QStringList *>(data)));
        break;
    case QMetaType::QVariantList:
        result = variantListToJS(*reinterpret_cast<const QVariantList *>(data));
        break;
    case QMetaType::QVariantMap:
        result = variantMapToJS(*reinterpret_cast<const QVariantMap *>(data));
        break;
    case QMetaType::QDateTime:
        result = QV4::Value::fromObject(m_v4Engine->newDateObject(*reinterpret_cast<const QDateTime *>(data)));
        break;
    case QMetaType::QDate:
        result = QV4::Value::fromObject(m_v4Engine->newDateObject(QDateTime(*reinterpret_cast<const QDate *>(data))));
        break;
    case QMetaType::QRegExp:
        result = QV4::Value::fromObject(m_v4Engine->newRegExpObject(*reinterpret_cast<const QRegExp *>(data)));
        break;
    case QMetaType::QObjectStar:
        result = newQObject(*reinterpret_cast<QObject* const *>(data));
        break;
    case QMetaType::QVariant:
        result = variantToJS(*reinterpret_cast<const QVariant*>(data));
        break;
    case QMetaType::QJsonValue:
        result = m_jsonWrapper.fromJsonValue(*reinterpret_cast<const QJsonValue *>(data));
        break;
    case QMetaType::QJsonObject:
        result = m_jsonWrapper.fromJsonObject(*reinterpret_cast<const QJsonObject *>(data));
        break;
    case QMetaType::QJsonArray:
        result = m_jsonWrapper.fromJsonArray(*reinterpret_cast<const QJsonArray *>(data));
        break;
    default:
        if (type == qMetaTypeId<QJSValue>()) {
            return QJSValuePrivate::get(*reinterpret_cast<const QJSValue*>(data))->value;
        } else {
            QByteArray typeName = QMetaType::typeName(type);
            if (typeName.endsWith('*') && !*reinterpret_cast<void* const *>(data)) {
                return QV4::Value::nullValue();
            } else {
                // Fall back to wrapping in a QVariant.
                result = QV4::Value::fromObject(m_v4Engine->newVariantObject(QVariant(type, data)));
            }
        }
    }
    return result;
}

// Converts a JS value to a meta-type.
// data must point to a place that can store a value of the given type.
// Returns true if conversion succeeded, false otherwise.
bool QV8Engine::metaTypeFromJS(const QV4::Value &value, int type, void *data) {
    // check if it's one of the types we know
    switch (QMetaType::Type(type)) {
    case QMetaType::Bool:
        *reinterpret_cast<bool*>(data) = value.toBoolean();
        return true;
    case QMetaType::Int:
        *reinterpret_cast<int*>(data) = value.toInt32();
        return true;
    case QMetaType::UInt:
        *reinterpret_cast<uint*>(data) = value.toUInt32();
        return true;
    case QMetaType::LongLong:
        *reinterpret_cast<qlonglong*>(data) = qlonglong(value.toInteger());
        return true;
    case QMetaType::ULongLong:
        *reinterpret_cast<qulonglong*>(data) = qulonglong(value.toInteger());
        return true;
    case QMetaType::Double:
        *reinterpret_cast<double*>(data) = value.toNumber();
        return true;
    case QMetaType::QString:
        if (value.isUndefined() || value.isNull())
            *reinterpret_cast<QString*>(data) = QString();
        else
            *reinterpret_cast<QString*>(data) = value.toString(m_v4Engine->current)->toQString();
        return true;
    case QMetaType::Float:
        *reinterpret_cast<float*>(data) = value.toNumber();
        return true;
    case QMetaType::Short:
        *reinterpret_cast<short*>(data) = short(value.toInt32());
        return true;
    case QMetaType::UShort:
        *reinterpret_cast<unsigned short*>(data) = value.toUInt16();
        return true;
    case QMetaType::Char:
        *reinterpret_cast<char*>(data) = char(value.toInt32());
        return true;
    case QMetaType::UChar:
        *reinterpret_cast<unsigned char*>(data) = (unsigned char)(value.toInt32());
        return true;
    case QMetaType::QChar:
        if (value.isString()) {
            QString str = value.stringValue()->toQString();
            *reinterpret_cast<QChar*>(data) = str.isEmpty() ? QChar() : str.at(0);
        } else {
            *reinterpret_cast<QChar*>(data) = QChar(ushort(value.toUInt16()));
        }
        return true;
    case QMetaType::QDateTime:
        if (QV4::DateObject *d = value.asDateObject()) {
            *reinterpret_cast<QDateTime *>(data) = d->toQDateTime();
            return true;
        } break;
    case QMetaType::QDate:
        if (QV4::DateObject *d = value.asDateObject()) {
            *reinterpret_cast<QDate *>(data) = d->toQDateTime().date();
            return true;
        } break;
    case QMetaType::QRegExp:
        if (QV4::RegExpObject *r = value.asRegExpObject()) {
            *reinterpret_cast<QRegExp *>(data) = r->toQRegExp();
            return true;
        } break;
    case QMetaType::QObjectStar: {
        if (isQObject(value) || value.isNull()) {
            *reinterpret_cast<QObject* *>(data) = qtObjectFromJS(value);
            return true;
        } break;
    }
    case QMetaType::QStringList:
        if (QV4::ArrayObject *a = value.asArrayObject()) {
            *reinterpret_cast<QStringList *>(data) = a->toQStringList();
            return true;
        } break;
    case QMetaType::QVariantList:
        if (QV4::ArrayObject *a = value.asArrayObject()) {
            *reinterpret_cast<QVariantList *>(data) = variantListFromJS(a);
            return true;
        } break;
    case QMetaType::QVariantMap:
        if (QV4::Object *o = value.asObject()) {
            *reinterpret_cast<QVariantMap *>(data) = variantMapFromJS(o);
            return true;
        } break;
    case QMetaType::QVariant:
        *reinterpret_cast<QVariant*>(data) = variantFromJS(value);
        return true;
    case QMetaType::QJsonValue:
        *reinterpret_cast<QJsonValue *>(data) = jsonValueFromJS(value);
        return true;
    case QMetaType::QJsonObject:
        *reinterpret_cast<QJsonObject *>(data) = jsonObjectFromJS(value);
        return true;
    case QMetaType::QJsonArray:
        *reinterpret_cast<QJsonArray *>(data) = jsonArrayFromJS(value);
        return true;
    default:
    ;
    }

#if 0
    if (isQtVariant(value)) {
        const QVariant &var = variantValue(value);
        // ### Enable once constructInPlace() is in qt master.
        if (var.userType() == type) {
            QMetaType::constructInPlace(type, data, var.constData());
            return true;
        }
        if (var.canConvert(type)) {
            QVariant vv = var;
            vv.convert(type);
            Q_ASSERT(vv.userType() == type);
            QMetaType::constructInPlace(type, data, vv.constData());
            return true;
        }

    }
#endif

    // Try to use magic; for compatibility with qscriptvalue_cast.

    QByteArray name = QMetaType::typeName(type);
    if (convertToNativeQObject(value, name, reinterpret_cast<void* *>(data)))
        return true;
    if (value.asVariantObject() && name.endsWith('*')) {
        int valueType = QMetaType::type(name.left(name.size()-1));
        QVariant &var = value.asVariantObject()->data;
        if (valueType == var.userType()) {
            // We have T t, T* is requested, so return &t.
            *reinterpret_cast<void* *>(data) = var.data();
            return true;
        } else if (QV4::Object *o = value.asObject()) {
            // Look in the prototype chain.
            QV4::Object *proto = o->prototype;
            while (proto) {
                bool canCast = false;
                if (QV4::VariantObject *vo = proto->asVariantObject()) {
                    const QVariant &v = vo->data;
                    canCast = (type == v.userType()) || (valueType && (valueType == v.userType()));
                }
                else if (isQObject(QV4::Value::fromObject(proto))) {
                    QByteArray className = name.left(name.size()-1);
                    if (QObject *qobject = qtObjectFromJS(QV4::Value::fromObject(proto)))
                        canCast = qobject->qt_metacast(className) != 0;
                }
                if (canCast) {
                    QByteArray varTypeName = QMetaType::typeName(var.userType());
                    if (varTypeName.endsWith('*'))
                        *reinterpret_cast<void* *>(data) = *reinterpret_cast<void* *>(var.data());
                    else
                        *reinterpret_cast<void* *>(data) = var.data();
                    return true;
                }
                proto = proto->prototype;
            }
        }
    } else if (value.isNull() && name.endsWith('*')) {
        *reinterpret_cast<void* *>(data) = 0;
        return true;
    } else if (type == qMetaTypeId<QJSValue>()) {
        *reinterpret_cast<QJSValue*>(data) = QJSValuePrivate::get(new QJSValuePrivate(m_v4Engine, value));
        return true;
    }

    return false;
}

// Converts a QVariant to JS.
QV4::Value QV8Engine::variantToJS(const QVariant &value)
{
    return metaTypeToJS(value.userType(), value.constData());
}

// Converts a JS value to a QVariant.
// Undefined -> QVariant() (invalid)
// Null -> QVariant((void*)0)
// Boolean -> QVariant(bool)
// Number -> QVariant(double)
// String -> QVariant(QString)
// Array -> QVariantList(...)
// Date -> QVariant(QDateTime)
// RegExp -> QVariant(QRegExp)
// [Any other object] -> QVariantMap(...)
QVariant QV8Engine::variantFromJS(const QV4::Value &value,
                                  V8ObjectSet &visitedObjects)
{
    Q_ASSERT(!value.isEmpty());
    if (value.isUndefined())
        return QVariant();
    if (value.isNull())
        return QVariant(QMetaType::VoidStar, 0);
    if (value.isBoolean())
        return value.booleanValue();
    if (value.isInteger())
        return value.integerValue();
    if (value.isNumber())
        return value.asDouble();
    if (value.isString())
        return value.stringValue()->toQString();
    Q_ASSERT(value.isObject());
    if (QV4::ArrayObject *a = value.asArrayObject())
        return variantListFromJS(a, visitedObjects);
    if (QV4::DateObject *d = value.asDateObject())
        return d->toQDateTime();
    if (QV4::RegExpObject *re = value.asRegExpObject())
        return re->toQRegExp();
    if (QV4::VariantObject *v = value.asVariantObject())
        return v->data;
    if (isQObject(value))
        return qVariantFromValue(qtObjectFromJS(value));
    if (isValueType(value))
        return toValueType(value);
    return variantMapFromJS(value.asObject(), visitedObjects);
}

QV4::Value QV8Engine::jsonValueToJS(const QJsonValue &value)
{
    return m_jsonWrapper.fromJsonValue(value);
}

QJsonValue QV8Engine::jsonValueFromJS(const QV4::Value &value)
{
    return m_jsonWrapper.toJsonValue(value);
}

QV4::Value QV8Engine::jsonObjectToJS(const QJsonObject &object)
{
    return m_jsonWrapper.fromJsonObject(object);
}

QJsonObject QV8Engine::jsonObjectFromJS(const QV4::Value &value)
{
    return m_jsonWrapper.toJsonObject(value.asObject());
}

QV4::Value QV8Engine::jsonArrayToJS(const QJsonArray &array)
{
    return m_jsonWrapper.fromJsonArray(array);
}

QJsonArray QV8Engine::jsonArrayFromJS(const QV4::Value &value)
{
    return m_jsonWrapper.toJsonArray(value.asArrayObject());
}

bool QV8Engine::convertToNativeQObject(const QV4::Value &value, const QByteArray &targetType, void **result)
{
    if (!targetType.endsWith('*'))
        return false;
    if (QObject *qobject = qtObjectFromJS(value)) {
        int start = targetType.startsWith("const ") ? 6 : 0;
        QByteArray className = targetType.mid(start, targetType.size()-start-1);
        if (void *instance = qobject->qt_metacast(className)) {
            *result = instance;
            return true;
        }
    }
    return false;
}

QObject *QV8Engine::qtObjectFromJS(const QV4::Value &value)
{
    if (!value.isObject())
        return 0;


    if (QV4::VariantObject *v = value.asVariantObject()) {
        QVariant variant = v->data;
        int type = variant.userType();
        if (type == QMetaType::QObjectStar)
            return *reinterpret_cast<QObject* const *>(variant.constData());
    }
    QV4::QObjectWrapper *wrapper = value.asQObjectWrapper();
    if (!wrapper)
        return 0;
    return wrapper->object;
}

void QV8Engine::startTimer(const QString &timerName)
{
    if (!m_time.isValid())
        m_time.start();
    m_startedTimers[timerName] = m_time.elapsed();
}

qint64 QV8Engine::stopTimer(const QString &timerName, bool *wasRunning)
{
    if (!m_startedTimers.contains(timerName)) {
        *wasRunning = false;
        return 0;
    }
    *wasRunning = true;
    qint64 startedAt = m_startedTimers.take(timerName);
    return m_time.elapsed() - startedAt;
}

int QV8Engine::consoleCountHelper(const QString &file, quint16 line, quint16 column)
{
    const QString key = file + QString::number(line) + QString::number(column);
    int number = m_consoleCount.value(key, 0);
    number++;
    m_consoleCount.insert(key, number);
    return number;
}

void QV8GCCallback::registerGcPrologueCallback()
{
    QV8Engine::ThreadData *td = QV8Engine::threadData();
    if (!td->gcPrologueCallbackRegistered) {
        td->gcPrologueCallbackRegistered = true;
        v8::V8::AddGCPrologueCallback(QV8GCCallback::garbageCollectorPrologueCallback, v8::kGCTypeMarkSweepCompact);
    }
}

QV8GCCallback::Node::Node(PrologueCallback callback)
    : prologueCallback(callback)
{
}

QV8GCCallback::Node::~Node()
{
    node.remove();
}

/*
   Ensure that each persistent handle is strong if it has CPP ownership
   and has no implicitly JS owned object owner in its parent chain, and
   weak otherwise.

   Any weak handle whose parent object is still alive will have an implicit
   reference (between the parent and the handle) added, so that it will
   not be collected.

   Note that this callback is registered only for kGCTypeMarkSweepCompact
   collection cycles, as it is during collection cycles of that type
   in which weak persistent handle callbacks are called when required.
 */
void QV8GCCallback::garbageCollectorPrologueCallback(v8::GCType, v8::GCCallbackFlags)
{
    if (!QV8Engine::hasThreadData())
        return;

    QV8Engine::ThreadData *td = QV8Engine::threadData();
    QV8GCCallback::Node *currNode = td->gcCallbackNodes.first();

    while (currNode) {
        // The client which adds itself to the list is responsible
        // for maintaining the correct implicit references in the
        // specified callback.
        currNode->prologueCallback(currNode);
        currNode = td->gcCallbackNodes.next(currNode);
    }
}

void QV8GCCallback::addGcCallbackNode(QV8GCCallback::Node *node)
{
    QV8Engine::ThreadData *td = QV8Engine::threadData();
    td->gcCallbackNodes.insert(node);
}

QV8Engine::ThreadData::ThreadData()
    : gcPrologueCallbackRegistered(false)
{
}

QV8Engine::ThreadData::~ThreadData()
{
}

QV4::Value QV8Engine::toString(const QString &string)
{
    return QV4::Value::fromString(m_v4Engine->newString(string));
}

QV4::Value QV8Engine::newValueType(QObject *object, int property, QQmlValueType *type)
{
    return QV4::QmlValueTypeWrapper::create(this, object, property, type);
}

QV4::Value QV8Engine::newValueType(const QVariant &value, QQmlValueType *type)
{
    return QV4::QmlValueTypeWrapper::create(this, value, type);
}

bool QV8Engine::isValueType(const QV4::Value &value) const
{
    return value.isObject() ? value.objectValue()->asQmlValueTypeWrapper() : 0;
}

QVariant QV8Engine::toValueType(const QV4::Value &obj)
{
    return obj.isObject() ? obj.objectValue()->asQmlValueTypeWrapper()->toVariant() : QVariant();
}

QT_END_NAMESPACE

