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

#include "qv8engine_p.h"

#include "qv8contextwrapper_p.h"
#include "qv8valuetypewrapper_p.h"
#include "qv8include_p.h"
#include "../../../3rdparty/javascriptcore/DateMath.h"

#include <private/qdeclarativelist_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativecomponent_p.h>
#include <private/qdeclarativestringconverters_p.h>
#include <private/qdeclarativeapplication_p.h>

#include <QtDeclarative/qdeclarativecomponent.h>

#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qnumeric.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qfontdatabase.h>
#include <private/qdeclarativexmlhttprequest_p.h>
#include <private/qdeclarativesqldatabase_p.h>

#include "qscript_impl_p.h"
#include "qv8domerrors_p.h"

Q_DECLARE_METATYPE(QJSValue)
Q_DECLARE_METATYPE(QList<int>)


// XXX TODO: Need to check all the global functions will also work in a worker script where the 
// QDeclarativeEngine is not available
QT_BEGIN_NAMESPACE

static bool ObjectComparisonCallback(v8::Local<v8::Object> lhs, v8::Local<v8::Object> rhs)
{
    if (lhs == rhs)
        return true;

    QV8ObjectResource *lhsr = static_cast<QV8ObjectResource*>(lhs->GetExternalResource());
    QV8ObjectResource *rhsr = static_cast<QV8ObjectResource*>(rhs->GetExternalResource());

    Q_ASSERT(lhsr->engine == rhsr->engine);

    if (lhsr && rhsr) {
        QV8ObjectResource::ResourceType lhst = lhsr->resourceType();
        QV8ObjectResource::ResourceType rhst = rhsr->resourceType();

        switch (lhst) {
        case QV8ObjectResource::ValueTypeType:
            if (rhst == QV8ObjectResource::ValueTypeType) {
                return lhsr->engine->valueTypeWrapper()->isEqual(lhsr, lhsr->engine->valueTypeWrapper()->toVariant(rhsr));
            } else if (rhst == QV8ObjectResource::VariantType) {
                return lhsr->engine->valueTypeWrapper()->isEqual(lhsr, lhsr->engine->variantWrapper()->toVariant(rhsr));
            }
            break;
        case QV8ObjectResource::VariantType:
            if (rhst == QV8ObjectResource::VariantType) {
                return lhsr->engine->variantWrapper()->toVariant(lhsr) == 
                       lhsr->engine->variantWrapper()->toVariant(rhsr);
            } else if (rhst == QV8ObjectResource::ValueTypeType) {
                return rhsr->engine->valueTypeWrapper()->isEqual(rhsr, rhsr->engine->variantWrapper()->toVariant(lhsr));
            }
            break;
        default:
            break;
        }
    }

    return false;
}

QV8Engine::QV8Engine(QJSEngine* qq, QJSEngine::ContextOwnership ownership)
    : q(qq)
    , m_engine(0)
    , m_ownsV8Context(ownership == QJSEngine::CreateNewContext)
    , m_context((ownership == QJSEngine::CreateNewContext) ? v8::Context::New() : v8::Persistent<v8::Context>::New(v8::Context::GetCurrent()))
    , m_originalGlobalObject(m_context)
    , m_xmlHttpRequestData(0)
    , m_sqlDatabaseData(0)
    , m_listModelData(0)
{
    qMetaTypeId<QJSValue>();
    qMetaTypeId<QList<int> >();

    QByteArray v8args = qgetenv("V8ARGS");
    if (!v8args.isEmpty())
        v8::V8::SetFlagsFromString(v8args.constData(), v8args.length());

    v8::HandleScope handle_scope;
    qPersistentRegister(m_context);
    v8::Context::Scope context_scope(m_context);

    v8::V8::SetUserObjectComparisonCallbackFunction(ObjectComparisonCallback);

    m_stringWrapper.init();
    m_contextWrapper.init(this);
    m_qobjectWrapper.init(this);
    m_typeWrapper.init(this);
    m_listWrapper.init(this);
    m_variantWrapper.init(this);
    m_valueTypeWrapper.init(this);

    {
    v8::Handle<v8::Value> v = global()->Get(v8::String::New("Object"))->ToObject()->Get(v8::String::New("getOwnPropertyNames"));
    m_getOwnPropertyNames = qPersistentNew<v8::Function>(v8::Handle<v8::Function>::Cast(v));
    }
}

QV8Engine::~QV8Engine()
{
    for (int ii = 0; ii < m_extensionData.count(); ++ii) 
        delete m_extensionData[ii];
    m_extensionData.clear();

    qt_rem_qmlsqldatabase(this, m_sqlDatabaseData); 
    m_sqlDatabaseData = 0;
    qt_rem_qmlxmlhttprequest(this, m_xmlHttpRequestData); 
    m_xmlHttpRequestData = 0;
    delete m_listModelData;
    m_listModelData = 0;

    qPersistentDispose(m_freezeObject);
    qPersistentDispose(m_getOwnPropertyNames);

    invalidateAllValues();
    clearExceptions();

    m_valueTypeWrapper.destroy();
    m_variantWrapper.destroy();
    m_listWrapper.destroy();
    m_typeWrapper.destroy();
    m_qobjectWrapper.destroy();
    m_contextWrapper.destroy();
    m_stringWrapper.destroy();

    m_originalGlobalObject.destroy();

    if (m_ownsV8Context)
        qPersistentDispose(m_context);
}

QString QV8Engine::toStringStatic(v8::Handle<v8::Value> jsstr)
{
    return toStringStatic(jsstr->ToString());
}

QString QV8Engine::toStringStatic(v8::Handle<v8::String> jsstr)
{
    QString qstr;
    qstr.resize(jsstr->Length());
    jsstr->Write((uint16_t*)qstr.data());
    return qstr;
}

QVariant QV8Engine::toVariant(v8::Handle<v8::Value> value, int typeHint)
{
    if (value.IsEmpty()) 
        return QVariant();

    if (typeHint == QVariant::Bool)
        return QVariant(value->BooleanValue());

    if (value->IsObject()) {
        QV8ObjectResource *r = (QV8ObjectResource *)value->ToObject()->GetExternalResource();
        if (r) {
            switch (r->resourceType()) {
            case QV8ObjectResource::ContextType:
            case QV8ObjectResource::TypeType:
            case QV8ObjectResource::XMLHttpRequestType:
            case QV8ObjectResource::DOMNodeType:
            case QV8ObjectResource::SQLDatabaseType:
            case QV8ObjectResource::ListModelType:
            case QV8ObjectResource::Context2DType:
            case QV8ObjectResource::ParticleDataType:
                return QVariant();
            case QV8ObjectResource::QObjectType:
                return qVariantFromValue<QObject *>(m_qobjectWrapper.toQObject(r));
            case QV8ObjectResource::ListType:
                return m_listWrapper.toVariant(r);
            case QV8ObjectResource::VariantType:
                return m_variantWrapper.toVariant(r);
            case QV8ObjectResource::ValueTypeType:
                return m_valueTypeWrapper.toVariant(r);
            }
        }
    }

    if (typeHint == qMetaTypeId<QList<QObject *> >() && value->IsArray()) {
        v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(value);

        QList<QObject *> list;
        uint32_t length = array->Length();
        for (uint32_t ii = 0; ii < length; ++ii) {
            v8::Local<v8::Value> arrayItem = array->Get(ii);
            if (arrayItem->IsObject()) {
                list << toQObject(arrayItem->ToObject());
            } else {
                list << 0;
            }
        }

        return qVariantFromValue<QList<QObject*> >(list);
    }

    return toBasicVariant(value);
}

static v8::Handle<v8::Array> arrayFromStringList(QV8Engine *engine, const QStringList &list)
{
    v8::Context::Scope scope(engine->context());
    v8::Local<v8::Array> result = v8::Array::New(list.count());
    for (int ii = 0; ii < list.count(); ++ii)
        result->Set(ii, engine->toString(list.at(ii)));
    return result;
}

static v8::Handle<v8::Array> arrayFromVariantList(QV8Engine *engine, const QVariantList &list)
{
    v8::Context::Scope scope(engine->context());
    v8::Local<v8::Array> result = v8::Array::New(list.count());
    for (int ii = 0; ii < list.count(); ++ii)
        result->Set(ii, engine->fromVariant(list.at(ii)));
    return result;
}

static v8::Handle<v8::Object> objectFromVariantMap(QV8Engine *engine, const QVariantMap &map)
{
    v8::Context::Scope scope(engine->context());
    v8::Local<v8::Object> object = v8::Object::New();
    for (QVariantMap::ConstIterator iter = map.begin(); iter != map.end(); ++iter) 
        object->Set(engine->toString(iter.key()), engine->fromVariant(iter.value()));
    return object;
}

Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &, QRegExp::PatternSyntax);

v8::Handle<v8::Value> QV8Engine::fromVariant(const QVariant &variant)
{
    int type = variant.userType();
    const void *ptr = variant.constData();

    if (type < QMetaType::User) {
        switch (QMetaType::Type(type)) {
            case QMetaType::Void:
                return v8::Undefined();
            case QMetaType::Bool:
                return v8::Boolean::New(*reinterpret_cast<const bool*>(ptr));
            case QMetaType::Int:
                return v8::Integer::New(*reinterpret_cast<const int*>(ptr));
            case QMetaType::UInt:
                return v8::Integer::NewFromUnsigned(*reinterpret_cast<const uint*>(ptr));
            case QMetaType::LongLong:
                return v8::Number::New(*reinterpret_cast<const qlonglong*>(ptr));
            case QMetaType::ULongLong:
                return v8::Number::New(*reinterpret_cast<const qulonglong*>(ptr));
            case QMetaType::Double:
                return v8::Number::New(*reinterpret_cast<const double*>(ptr));
            case QMetaType::QString:
                return m_stringWrapper.toString(*reinterpret_cast<const QString*>(ptr));
            case QMetaType::Float:
                return v8::Number::New(*reinterpret_cast<const float*>(ptr));
            case QMetaType::Short:
                return v8::Integer::New(*reinterpret_cast<const short*>(ptr));
            case QMetaType::UShort:
                return v8::Integer::NewFromUnsigned(*reinterpret_cast<const unsigned short*>(ptr));
            case QMetaType::Char:
                return v8::Integer::New(*reinterpret_cast<const char*>(ptr));
            case QMetaType::UChar:
                return v8::Integer::NewFromUnsigned(*reinterpret_cast<const unsigned char*>(ptr));
            case QMetaType::QChar:
                return v8::Integer::New((*reinterpret_cast<const QChar*>(ptr)).unicode());
            case QMetaType::QDateTime:
                return v8::Date::New(qtDateTimeToJsDate(*reinterpret_cast<const QDateTime *>(ptr)));
            case QMetaType::QDate:
                return v8::Date::New(qtDateTimeToJsDate(QDateTime(*reinterpret_cast<const QDate *>(ptr))));
            case QMetaType::QTime:
                return v8::Date::New(qtDateTimeToJsDate(QDateTime(QDate(1970,1,1), *reinterpret_cast<const QTime *>(ptr))));
            case QMetaType::QRegExp:
                return QJSConverter::toRegExp(*reinterpret_cast<const QRegExp *>(ptr));
            case QMetaType::QObjectStar:
            case QMetaType::QWidgetStar:
                return newQObject(*reinterpret_cast<QObject* const *>(ptr));
            case QMetaType::QStringList: 
                return arrayFromStringList(this, *reinterpret_cast<const QStringList *>(ptr));
            case QMetaType::QVariantList:
                return arrayFromVariantList(this, *reinterpret_cast<const QVariantList *>(ptr));
            case QMetaType::QVariantMap:
                return objectFromVariantMap(this, *reinterpret_cast<const QVariantMap *>(ptr));

            default:
                break;
        }

        if (m_engine) {
            if (QDeclarativeValueType *vt = QDeclarativeEnginePrivate::get(m_engine)->valueTypes[type])
                return m_valueTypeWrapper.newValueType(variant, vt);
        }

    } else {
        if (type == qMetaTypeId<QDeclarativeListReference>()) {
            typedef QDeclarativeListReferencePrivate QDLRP;
            QDLRP *p = QDLRP::get((QDeclarativeListReference*)ptr);
            if (p->object) {
                return m_listWrapper.newList(p->property, p->propertyType);
            } else {
                return v8::Null();
            }
        } else if (type == qMetaTypeId<QList<QObject *> >()) {
            // XXX Can this be made more by using Array as a prototype and implementing
            // directly against QList<QObject*>?
            const QList<QObject *> &list = *(QList<QObject *>*)ptr;
            v8::Local<v8::Array> array = v8::Array::New(list.count());
            for (int ii = 0; ii < list.count(); ++ii) 
                array->Set(ii, newQObject(list.at(ii)));
            return array;
        } 

        bool objOk;
        QObject *obj = QDeclarativeMetaType::toQObject(variant, &objOk);
        if (objOk) 
            return newQObject(obj);
    }

    // XXX TODO: To be compatible, we still need to handle:
    //    + QJSValue
    //    + QObjectList
    //    + QList<int>

    return m_variantWrapper.newVariant(variant);
}

// A handle scope and context must be entered
v8::Local<v8::Script> QV8Engine::qmlModeCompile(const QString &source, const QString &fileName, int lineNumber)
{
    v8::Local<v8::String> v8source = m_stringWrapper.toString(source);
    v8::Local<v8::String> v8fileName = m_stringWrapper.toString(fileName);

    v8::ScriptOrigin origin(v8fileName, v8::Integer::New(lineNumber - 1));

    v8::Local<v8::Script> script = v8::Script::Compile(v8source, &origin, 0, v8::Handle<v8::String>(), 
                                                       v8::Script::QmlMode);

    return script;
}

QNetworkAccessManager *QV8Engine::networkAccessManager() 
{
    return QDeclarativeEnginePrivate::get(m_engine)->getNetworkAccessManager();
}

const QSet<QString> &QV8Engine::illegalNames() const
{
    return m_illegalNames;
}

// Requires a handle scope
v8::Local<v8::Array> QV8Engine::getOwnPropertyNames(v8::Handle<v8::Object> o)
{
    // FIXME Newer v8 have API for this function
    v8::TryCatch tc;
    v8::Handle<v8::Value> args[] = { o };
    v8::Local<v8::Value> r = m_getOwnPropertyNames->Call(global(), 1, args);
    if (tc.HasCaught())
        return v8::Array::New();
    else
        return v8::Local<v8::Array>::Cast(r);
}

QDeclarativeContextData *QV8Engine::callingContext()
{
    return m_contextWrapper.callingContext();
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
QVariant QV8Engine::toBasicVariant(v8::Handle<v8::Value> value)
{
    if (value->IsNull() || value->IsUndefined())
        return QVariant();
    if (value->IsBoolean())
        return value->ToBoolean()->Value();
    if (value->IsInt32())
        return value->ToInt32()->Value();
    if (value->IsNumber())
        return value->ToNumber()->Value();
    if (value->IsString())
        return m_stringWrapper.toString(value->ToString());
    if (value->IsDate())
        return qtDateTimeFromJsDate(v8::Handle<v8::Date>::Cast(value)->NumberValue());
    // NOTE: since we convert QTime to JS Date, round trip will change the variant type (to QDateTime)!

    Q_ASSERT(value->IsObject());

    if (value->IsRegExp()) {
        v8::Context::Scope scope(context());
        return QJSConverter::toRegExp(v8::Handle<v8::RegExp>::Cast(value));
    }
    if (value->IsArray()) {
        v8::Context::Scope scope(context());
        QVariantList rv;

        v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(value);
        int length = array->Length();
        for (int ii = 0; ii < length; ++ii)
            rv << toVariant(array->Get(ii), -1);

        return rv;
    }
    if (!value->IsFunction()) {
        v8::Context::Scope scope(context());
        v8::Handle<v8::Object> object = value->ToObject();
        v8::Local<v8::Array> properties = object->GetPropertyNames();
        int length = properties->Length();
        if (length == 0)
            return QVariant();

        QVariantMap map; 
        for (int ii = 0; ii < length; ++ii) {
            v8::Handle<v8::Value> property = properties->Get(ii);
            map.insert(toString(property), toVariant(object->Get(property), -1));
        }
        return map;
    }

    return QVariant();
}



#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

struct StaticQtMetaObject : public QObject
{
    static const QMetaObject *get()
        { return &static_cast<StaticQtMetaObject*> (0)->staticQtMetaObject; }
};

void QV8Engine::initializeGlobal(v8::Handle<v8::Object> global)
{
    v8::Local<v8::Function> printFn = V8FUNCTION(print, this);

    v8::Local<v8::Object> console = v8::Object::New();
    console->Set(v8::String::New("log"), printFn);
    console->Set(v8::String::New("debug"), printFn);

    v8::Local<v8::Object> qt = v8::Object::New();

    // Set all the enums from the "Qt" namespace
    const QMetaObject *qtMetaObject = StaticQtMetaObject::get();
    for (int ii = 0; ii < qtMetaObject->enumeratorCount(); ++ii) {
        QMetaEnum enumerator = qtMetaObject->enumerator(ii);
        for (int jj = 0; jj < enumerator.keyCount(); ++jj) {
            qt->Set(v8::String::New(enumerator.key(jj)), v8::Integer::New(enumerator.value(jj)));
        }
    }

    qt->Set(v8::String::New("include"), V8FUNCTION(QV8Include::include, this));
    qt->Set(v8::String::New("isQtObject"), V8FUNCTION(isQtObject, this));
    qt->Set(v8::String::New("rgba"), V8FUNCTION(rgba, this));
    qt->Set(v8::String::New("hsla"), V8FUNCTION(hsla, this));
    qt->Set(v8::String::New("rect"), V8FUNCTION(rect, this));
    qt->Set(v8::String::New("point"), V8FUNCTION(point, this));
    qt->Set(v8::String::New("size"), V8FUNCTION(size, this));
    qt->Set(v8::String::New("vector3d"), V8FUNCTION(vector3d, this));
    qt->Set(v8::String::New("vector4d"), V8FUNCTION(vector4d, this));

    qt->Set(v8::String::New("formatDate"), V8FUNCTION(formatDate, this));
    qt->Set(v8::String::New("formatTime"), V8FUNCTION(formatTime, this));
    qt->Set(v8::String::New("formatDateTime"), V8FUNCTION(formatDateTime, this));

    qt->Set(v8::String::New("openUrlExternally"), V8FUNCTION(openUrlExternally, this));
    qt->Set(v8::String::New("fontFamilies"), V8FUNCTION(fontFamilies, this));
    qt->Set(v8::String::New("md5"), V8FUNCTION(md5, this));
    qt->Set(v8::String::New("btoa"), V8FUNCTION(btoa, this));
    qt->Set(v8::String::New("atob"), V8FUNCTION(atob, this));
    qt->Set(v8::String::New("resolvedUrl"), V8FUNCTION(resolvedUrl, this));

    if (m_engine) {
        qt->Set(v8::String::New("application"), newQObject(new QDeclarativeApplication(m_engine)));
        qt->Set(v8::String::New("lighter"), V8FUNCTION(lighter, this));
        qt->Set(v8::String::New("darker"), V8FUNCTION(darker, this));
        qt->Set(v8::String::New("tint"), V8FUNCTION(tint, this));
        qt->Set(v8::String::New("quit"), V8FUNCTION(quit, this));
        qt->Set(v8::String::New("createQmlObject"), V8FUNCTION(createQmlObject, this));
        qt->Set(v8::String::New("createComponent"), V8FUNCTION(createComponent, this));
    }

    global->Set(v8::String::New("qsTranslate"), V8FUNCTION(qsTranslate, this));
    global->Set(v8::String::New("QT_TRANSLATE_NOOP"), V8FUNCTION(qsTranslateNoOp, this));
    global->Set(v8::String::New("qsTr"), V8FUNCTION(qsTr, this));
    global->Set(v8::String::New("QT_TR_NOOP"), V8FUNCTION(qsTrNoOp, this));
    global->Set(v8::String::New("qsTrId"), V8FUNCTION(qsTrId, this));
    global->Set(v8::String::New("QT_TRID_NOOP"), V8FUNCTION(qsTrIdNoOp, this));

    global->Set(v8::String::New("print"), printFn);
    global->Set(v8::String::New("console"), console);
    global->Set(v8::String::New("Qt"), qt);
    global->Set(v8::String::New("gc"), V8FUNCTION(gc, this));

    v8::Local<v8::Object> string = v8::Local<v8::Object>::Cast(global->Get(v8::String::New("String")));
    v8::Local<v8::Object> stringPrototype = v8::Local<v8::Object>::Cast(string->Get(v8::String::New("prototype")));
    stringPrototype->Set(v8::String::New("arg"), V8FUNCTION(stringArg, this));

    qt_add_domexceptions(this);
    m_xmlHttpRequestData = qt_add_qmlxmlhttprequest(this);
    m_sqlDatabaseData = qt_add_qmlsqldatabase(this);

    {
    v8::Handle<v8::Value> args[] = { global };
    v8::Local<v8::Value> names = m_getOwnPropertyNames->Call(global, 1, args);
    v8::Local<v8::Array> namesArray = v8::Local<v8::Array>::Cast(names);
    for (quint32 ii = 0; ii < namesArray->Length(); ++ii) 
        m_illegalNames.insert(toString(namesArray->Get(ii)));
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

    v8::Local<v8::Script> freeze = v8::Script::New(v8::String::New(FREEZE_SOURCE));
    v8::Local<v8::Value> result = freeze->Run();
    Q_ASSERT(result->IsFunction());
    m_freezeObject = qPersistentNew(v8::Local<v8::Function>::Cast(result));
#undef FREEZE_SOURCE
    }
}

void QV8Engine::freezeObject(v8::Handle<v8::Value> value)
{
    v8::Handle<v8::Value> args[] = { value };
    m_freezeObject->Call(global(), 1, args);
}

void QV8Engine::gc()
{
    v8::V8::LowMemoryNotification();
    while (!v8::V8::IdleNotification()) {}
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

v8::Handle<v8::Value> QV8Engine::gc(const v8::Arguments &args)
{
    Q_UNUSED(args);
    gc();
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8Engine::print(const v8::Arguments &args)
{
    QString result;
    for (int i = 0; i < args.Length(); ++i) {
        if (i != 0)
            result.append(QLatin1Char(' '));

        v8::Local<v8::String> jsstr = args[i]->ToString();
        if (!jsstr.IsEmpty()) {
            QString qstr;
            qstr.resize(jsstr->Length());
            jsstr->Write((uint16_t*)qstr.data());
            result.append(qstr);
        }
    }
    qDebug("%s", qPrintable(result));
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8Engine::stringArg(const v8::Arguments &args)
{
    QString value = V8ENGINE()->toString(args.This()->ToString());
    if (args.Length() != 1)
        V8THROW_ERROR("String.arg(): Invalid arguments");

    if (args[0]->IsUint32())
        return V8ENGINE()->toString(value.arg(args[0]->Uint32Value()));
    else if (args[0]->IsInt32())
        return V8ENGINE()->toString(value.arg(args[0]->Int32Value()));
    else if (args[0]->IsNumber())
        return V8ENGINE()->toString(value.arg(args[0]->NumberValue()));
    else if (args[0]->IsBoolean())
        return V8ENGINE()->toString(value.arg(args[0]->BooleanValue()));

    return V8ENGINE()->toString(value.arg(V8ENGINE()->toString(args[0])));
}
/*!
\qmlmethod bool Qt::isQtObject(object)
Returns true if \c object is a valid reference to a Qt or QML object, otherwise false.
*/
v8::Handle<v8::Value> QV8Engine::isQtObject(const v8::Arguments &args)
{
    if (args.Length() == 0)
        return v8::Boolean::New(false);

    return v8::Boolean::New(0 != V8ENGINE()->toQObject(args[0]));
}

/*!
\qmlmethod color Qt::rgba(real red, real green, real blue, real alpha)

Returns a color with the specified \c red, \c green, \c blue and \c alpha components.
All components should be in the range 0-1 inclusive.
*/
v8::Handle<v8::Value> QV8Engine::rgba(const v8::Arguments &args)
{
    int argCount = args.Length();
    if (argCount < 3 || argCount > 4)
        V8THROW_ERROR("Qt.rgba(): Invalid arguments");

    double r = args[0]->NumberValue();
    double g = args[1]->NumberValue();
    double b = args[2]->NumberValue();
    double a = (argCount == 4) ? args[3]->NumberValue() : 1;

    if (r < 0.0) r=0.0;
    if (r > 1.0) r=1.0;
    if (g < 0.0) g=0.0;
    if (g > 1.0) g=1.0;
    if (b < 0.0) b=0.0;
    if (b > 1.0) b=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return V8ENGINE()->fromVariant(QVariant::fromValue(QColor::fromRgbF(r, g, b, a)));
}

/*!
\qmlmethod color Qt::hsla(real hue, real saturation, real lightness, real alpha)

Returns a color with the specified \c hue, \c saturation, \c lightness and \c alpha components.
All components should be in the range 0-1 inclusive.
*/
v8::Handle<v8::Value> QV8Engine::hsla(const v8::Arguments &args)
{
    int argCount = args.Length();
    if (argCount < 3 || argCount > 4)
        V8THROW_ERROR("Qt.hsla(): Invalid arguments");

    double h = args[0]->NumberValue();
    double s = args[1]->NumberValue();
    double l = args[2]->NumberValue();
    double a = (argCount == 4) ? args[3]->NumberValue() : 1;

    if (h < 0.0) h=0.0;
    if (h > 1.0) h=1.0;
    if (s < 0.0) s=0.0;
    if (s > 1.0) s=1.0;
    if (l < 0.0) l=0.0;
    if (l > 1.0) l=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return V8ENGINE()->fromVariant(QVariant::fromValue(QColor::fromHslF(h, s, l, a)));
}

/*!
\qmlmethod rect Qt::rect(int x, int y, int width, int height) 

Returns a \c rect with the top-left corner at \c x, \c y and the specified \c width and \c height.

The returned object has \c x, \c y, \c width and \c height attributes with the given values.
*/
v8::Handle<v8::Value> QV8Engine::rect(const v8::Arguments &args)
{
    if (args.Length() != 4)
        V8THROW_ERROR("Qt.rect(): Invalid arguments");

    double x = args[0]->NumberValue();
    double y = args[1]->NumberValue();
    double w = args[2]->NumberValue();
    double h = args[3]->NumberValue();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QRectF(x, y, w, h)));
}

/*!
\qmlmethod point Qt::point(int x, int y)
Returns a Point with the specified \c x and \c y coordinates.
*/
v8::Handle<v8::Value> QV8Engine::point(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.point(): Invalid arguments");

    double x = args[0]->ToNumber()->Value();
    double y = args[1]->ToNumber()->Value();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QPointF(x, y)));
}

/*!
\qmlmethod Qt::size(int width, int height)
Returns a Size with the specified \c width and \c height.
*/
v8::Handle<v8::Value> QV8Engine::size(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.size(): Invalid arguments");
    
    double w = args[0]->ToNumber()->Value();
    double h = args[1]->ToNumber()->Value();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QSizeF(w, h)));
}

/*!
\qmlmethod Qt::vector3d(real x, real y, real z)
Returns a Vector3D with the specified \c x, \c y and \c z.
*/
v8::Handle<v8::Value> QV8Engine::vector3d(const v8::Arguments &args)
{
    if (args.Length() != 3)
        V8THROW_ERROR("Qt.vector(): Invalid arguments");

    double x = args[0]->ToNumber()->Value();
    double y = args[1]->ToNumber()->Value();
    double z = args[2]->ToNumber()->Value();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QVector3D(x, y, z)));
}

/*!
\qmlmethod Qt::vector4d(real x, real y, real z, real w)
Returns a Vector4D with the specified \c x, \c y, \c z and \c w.
*/
v8::Handle<v8::Value> QV8Engine::vector4d(const v8::Arguments &args)
{
    if (args.Length() != 4)
        V8THROW_ERROR("Qt.vector4d(): Invalid arguments");

    double x = args[0]->NumberValue();
    double y = args[1]->NumberValue();
    double z = args[2]->NumberValue();
    double w = args[3]->NumberValue();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QVector4D(x, y, z, w)));
}

/*!
\qmlmethod color Qt::lighter(color baseColor, real factor)
Returns a color lighter than \c baseColor by the \c factor provided.

If the factor is greater than 1.0, this functions returns a lighter color.
Setting factor to 1.5 returns a color that is 50% brighter. If the factor is less than 1.0,
the return color is darker, but we recommend using the Qt.darker() function for this purpose.
If the factor is 0 or negative, the return value is unspecified.

The function converts the current RGB color to HSV, multiplies the value (V) component
by factor and converts the color back to RGB.

If \c factor is not supplied, returns a color 50% lighter than \c baseColor (factor 1.5).
*/
v8::Handle<v8::Value> QV8Engine::lighter(const v8::Arguments &args)
{
    if (args.Length() != 1 && args.Length() != 2)
        V8THROW_ERROR("Qt.lighter(): Invalid arguments");

    QColor color;
    QVariant v = V8ENGINE()->toVariant(args[0], -1);
    if (v.userType() == QVariant::Color) {
        color = v.value<QColor>();
    } else if (v.userType() == QVariant::String) {
        bool ok = false;
        color = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else {
        return v8::Null();
    }

    qreal factor = 1.5;
    if (args.Length() == 2)
        factor = args[1]->ToNumber()->Value();

    color = color.lighter(int(qRound(factor*100.)));
    return V8ENGINE()->fromVariant(QVariant::fromValue(color));
}

/*!
\qmlmethod color Qt::darker(color baseColor, real factor)
Returns a color darker than \c baseColor by the \c factor provided.

If the factor is greater than 1.0, this function returns a darker color.
Setting factor to 3.0 returns a color that has one-third the brightness.
If the factor is less than 1.0, the return color is lighter, but we recommend using
the Qt.lighter() function for this purpose. If the factor is 0 or negative, the return
value is unspecified.

The function converts the current RGB color to HSV, divides the value (V) component
by factor and converts the color back to RGB.

If \c factor is not supplied, returns a color 50% darker than \c baseColor (factor 2.0).
*/
v8::Handle<v8::Value> QV8Engine::darker(const v8::Arguments &args)
{
    if (args.Length() != 1 && args.Length() != 2)
        V8THROW_ERROR("Qt.darker(): Invalid arguments");

    QColor color;
    QVariant v = V8ENGINE()->toVariant(args[0], -1);
    if (v.userType() == QVariant::Color) {
        color = v.value<QColor>();
    } else if (v.userType() == QVariant::String) {
        bool ok = false;
        color = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else {
        return v8::Null();
    }

    qreal factor = 2.0;
    if (args.Length() == 2)
        factor = args[1]->ToNumber()->Value();

    color = color.darker(int(qRound(factor*100.)));
    return V8ENGINE()->fromVariant(QVariant::fromValue(color));
}

/*!
    \qmlmethod color Qt::tint(color baseColor, color tintColor)
    This function allows tinting one color with another.

    The tint color should usually be mostly transparent, or you will not be
    able to see the underlying color. The below example provides a slight red
    tint by having the tint color be pure red which is only 1/16th opaque.

    \qml
    Item {
        Rectangle {
            x: 0; width: 80; height: 80
            color: "lightsteelblue"
        }
        Rectangle {
            x: 100; width: 80; height: 80
            color: Qt.tint("lightsteelblue", "#10FF0000")
        }
    }
    \endqml
    \image declarative-rect_tint.png

    Tint is most useful when a subtle change is intended to be conveyed due to some event; you can then use tinting to more effectively tune the visible color.
*/
v8::Handle<v8::Value> QV8Engine::tint(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.tint(): Invalid arguments");

    // base color
    QColor color;
    QVariant v = V8ENGINE()->toVariant(args[0], -1);
    if (v.userType() == QVariant::Color) {
        color = v.value<QColor>();
    } else if (v.userType() == QVariant::String) {
        bool ok = false;
        color = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else {
        return v8::Null();
    }

    // tint color
    QColor tintColor;
    v = V8ENGINE()->toVariant(args[1], -1);
    if (v.userType() == QVariant::Color) {
        tintColor = v.value<QColor>();
    } else if (v.userType() == QVariant::String) {
        bool ok = false;
        tintColor = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else {
        return v8::Null();
    }

    // tint the base color and return the final color
    QColor finalColor;
    int a = tintColor.alpha();
    if (a == 0xFF)
        finalColor = tintColor;
    else if (a == 0x00)
        finalColor = color;
    else {
        qreal a = tintColor.alphaF();
        qreal inv_a = 1.0 - a;

        finalColor.setRgbF(tintColor.redF() * a + color.redF() * inv_a,
                           tintColor.greenF() * a + color.greenF() * inv_a,
                           tintColor.blueF() * a + color.blueF() * inv_a,
                           a + inv_a * color.alphaF());
    }

    return V8ENGINE()->fromVariant(QVariant::fromValue(finalColor));
}

/*!
\qmlmethod string Qt::formatDate(datetime date, variant format)

Returns a string representation of \c date, optionally formatted according
to \c format.

The \a date parameter may be a JavaScript \c Date object, a \l{date}{date}
property, a QDate, or QDateTime value. The \a format parameter may be any of
the possible format values as described for
\l{QML:Qt::formatDateTime()}{Qt.formatDateTime()}.

If \a format is not specified, \a date is formatted using
\l {Qt::DefaultLocaleShortDate}{Qt.DefaultLocaleShortDate}.
*/
v8::Handle<v8::Value> QV8Engine::formatDate(const v8::Arguments &args)
{
    if (args.Length() < 1 || args.Length() > 2)
        V8THROW_ERROR("Qt.formatDate(): Invalid arguments");

    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    QDate date = V8ENGINE()->toVariant(args[0], -1).toDateTime().date();
    QString formattedDate;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = V8ENGINE()->toVariant(args[1], -1).toString();
            formattedDate = date.toString(format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            Qt::DateFormat format = Qt::DateFormat(intFormat);
            formattedDate = date.toString(format);
        } else {
            V8THROW_ERROR("Qt.formatDate(): Invalid date format");
        }
    } else {
         formattedDate = date.toString(enumFormat);
    }

    return V8ENGINE()->fromVariant(QVariant::fromValue(formattedDate));
}

/*!
\qmlmethod string Qt::formatTime(datetime time, variant format)

Returns a string representation of \c time, optionally formatted according to
\c format.

The \a time parameter may be a JavaScript \c Date object, a QTime, or QDateTime
value. The \a format parameter may be any of the possible format values as
described for \l{QML:Qt::formatDateTime()}{Qt.formatDateTime()}.

If \a format is not specified, \a time is formatted using
\l {Qt::DefaultLocaleShortDate}{Qt.DefaultLocaleShortDate}.
*/
v8::Handle<v8::Value> QV8Engine::formatTime(const v8::Arguments &args)
{
    if (args.Length() < 1 || args.Length() > 2)
        V8THROW_ERROR("Qt.formatTime(): Invalid arguments");

    QVariant argVariant = V8ENGINE()->toVariant(args[0], -1);
    QTime time;
    if (args[0]->IsDate() || (argVariant.type() == QVariant::String))
        time = argVariant.toDateTime().time();
    else // if (argVariant.type() == QVariant::Time), or invalid.
        time = argVariant.toTime();

    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    QString formattedTime;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = V8ENGINE()->toVariant(args[1], -1).toString();
            formattedTime = time.toString(format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            Qt::DateFormat format = Qt::DateFormat(intFormat);
            formattedTime = time.toString(format);
        } else {
            V8THROW_ERROR("Qt.formatTime(): Invalid time format");
        }
    } else {
         formattedTime = time.toString(enumFormat);
    }

    return V8ENGINE()->fromVariant(QVariant::fromValue(formattedTime));
}

/*!
\qmlmethod string Qt::formatDateTime(datetime dateTime, variant format)

Returns a string representation of \c datetime, optionally formatted according to
\c format.

The \a date parameter may be a JavaScript \c Date object, a \l{date}{date}
property, a QDate, QTime, or QDateTime value.

If \a format is not provided, \a dateTime is formatted using
\l {Qt::DefaultLocaleShortDate}{Qt.DefaultLocaleShortDate}. Otherwise,
\a format should be either:

\list
\o One of the Qt::DateFormat enumeration values, such as
   \c Qt.DefaultLocaleShortDate or \c Qt.ISODate
\o A string that specifies the format of the returned string, as detailed below.
\endlist

If \a format specifies a format string, it should use the following expressions
to specify the date:

    \table
    \header \i Expression \i Output
    \row \i d \i the day as number without a leading zero (1 to 31)
    \row \i dd \i the day as number with a leading zero (01 to 31)
    \row \i ddd
            \i the abbreviated localized day name (e.g. 'Mon' to 'Sun').
            Uses QDate::shortDayName().
    \row \i dddd
            \i the long localized day name (e.g. 'Monday' to 'Qt::Sunday').
            Uses QDate::longDayName().
    \row \i M \i the month as number without a leading zero (1-12)
    \row \i MM \i the month as number with a leading zero (01-12)
    \row \i MMM
            \i the abbreviated localized month name (e.g. 'Jan' to 'Dec').
            Uses QDate::shortMonthName().
    \row \i MMMM
            \i the long localized month name (e.g. 'January' to 'December').
            Uses QDate::longMonthName().
    \row \i yy \i the year as two digit number (00-99)
    \row \i yyyy \i the year as four digit number
    \endtable

In addition the following expressions can be used to specify the time:

    \table
    \header \i Expression \i Output
    \row \i h
         \i the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \i hh
         \i the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \i m \i the minute without a leading zero (0 to 59)
    \row \i mm \i the minute with a leading zero (00 to 59)
    \row \i s \i the second without a leading zero (0 to 59)
    \row \i ss \i the second with a leading zero (00 to 59)
    \row \i z \i the milliseconds without leading zeroes (0 to 999)
    \row \i zzz \i the milliseconds with leading zeroes (000 to 999)
    \row \i AP
            \i use AM/PM display. \e AP will be replaced by either "AM" or "PM".
    \row \i ap
            \i use am/pm display. \e ap will be replaced by either "am" or "pm".
    \endtable

    All other input characters will be ignored. Any sequence of characters that
    are enclosed in single quotes will be treated as text and not be used as an
    expression. Two consecutive single quotes ("''") are replaced by a single quote
    in the output.

For example, if the following date/time value was specified:

    \code
    // 21 May 2001 14:13:09
    var dateTime = new Date(2001, 5, 21, 14, 13, 09)
    \endcode

This \a dateTime value could be passed to \c Qt.formatDateTime(),
\l {QML:Qt::formatDate()}{Qt.formatDate()} or \l {QML:Qt::formatTime()}{Qt.formatTime()}
with the \a format values below to produce the following results:

    \table
    \header \i Format \i Result
    \row \i "dd.MM.yyyy"      \i 21.05.2001
    \row \i "ddd MMMM d yy"   \i Tue May 21 01
    \row \i "hh:mm:ss.zzz"    \i 14:13:09.042
    \row \i "h:m:s ap"        \i 2:13:9 pm
    \endtable
*/
v8::Handle<v8::Value> QV8Engine::formatDateTime(const v8::Arguments &args)
{
    if (args.Length() < 1 || args.Length() > 2)
        V8THROW_ERROR("Qt.formatDateTime(): Invalid arguments");

    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    QDateTime dt = V8ENGINE()->toVariant(args[0], -1).toDateTime();
    QString formattedDt;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = V8ENGINE()->toVariant(args[1], -1).toString();
            formattedDt = dt.toString(format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            Qt::DateFormat format = Qt::DateFormat(intFormat);
            formattedDt = dt.toString(format);
        } else {
            V8THROW_ERROR("Qt.formatDateTime(): Invalid datetime format");
        }
    } else {
         formattedDt = dt.toString(enumFormat);
    }

    return V8ENGINE()->fromVariant(QVariant::fromValue(formattedDt));
}

double QV8Engine::qtDateTimeToJsDate(const QDateTime &dt)
{
    // from QScriptEngine::DateTimeToMs()
    if (!dt.isValid()) {
        return qSNaN();
    }
    QDateTime utc = dt.toUTC();
    QDate date = utc.date();
    QTime time = utc.time();
    QV8DateConverter::JSC::GregorianDateTime tm;
    tm.year = date.year() - 1900;
    tm.month = date.month() - 1;
    tm.monthDay = date.day();
    tm.weekDay = date.dayOfWeek();
    tm.yearDay = date.dayOfYear();
    tm.hour = time.hour();
    tm.minute = time.minute();
    tm.second = time.second();
    return QV8DateConverter::JSC::gregorianDateTimeToMS(tm, time.msec());
}

QDateTime QV8Engine::qtDateTimeFromJsDate(double jsDate)
{
    // from QScriptEngine::MsToDateTime()
    if (qIsNaN(jsDate))
        return QDateTime();
    QV8DateConverter::JSC::GregorianDateTime tm;
    QV8DateConverter::JSC::msToGregorianDateTime(jsDate, tm);

    // from QScriptEngine::MsFromTime()
    int ms = int(::fmod(jsDate, 1000.0));
    if (ms < 0)
        ms += int(1000.0);

    QDateTime convertedUTC = QDateTime(QDate(tm.year + 1900, tm.month + 1, tm.monthDay),
                                       QTime(tm.hour, tm.minute, tm.second, ms), Qt::UTC);
    return convertedUTC.toLocalTime();
}

/*!
\qmlmethod bool Qt::openUrlExternally(url target)
Attempts to open the specified \c target url in an external application, based on the user's desktop preferences. Returns true if it succeeds, and false otherwise.
*/
v8::Handle<v8::Value> QV8Engine::openUrlExternally(const v8::Arguments &args)
{
    if (args.Length() != 1)
        return V8ENGINE()->fromVariant(false);

    bool ret = false;
#ifndef QT_NO_DESKTOPSERVICES
    ret = QDesktopServices::openUrl(V8ENGINE()->toVariant(resolvedUrl(args), -1).toUrl());
#endif
    return V8ENGINE()->fromVariant(ret);
}

/*!
  \qmlmethod url Qt::resolvedUrl(url url)
  Returns \a url resolved relative to the URL of the caller.
*/
v8::Handle<v8::Value> QV8Engine::resolvedUrl(const v8::Arguments &args)
{
    QUrl url = V8ENGINE()->toVariant(args[0], -1).toUrl();
    QDeclarativeEngine *e = V8ENGINE()->engine();
    QDeclarativeEnginePrivate *p = 0;
    if (e) p = QDeclarativeEnginePrivate::get(e);
    if (p) {
        QDeclarativeContextData *ctxt = V8ENGINE()->callingContext();
        if (ctxt)
            return V8ENGINE()->fromVariant(ctxt->resolvedUrl(url));
        else
            return V8ENGINE()->fromVariant(url);
    }

    return V8ENGINE()->fromVariant(e->baseUrl().resolved(url));
}

/*!
\qmlmethod list<string> Qt::fontFamilies()
Returns a list of the font families available to the application.
*/
v8::Handle<v8::Value> QV8Engine::fontFamilies(const v8::Arguments &args)
{
    if (args.Length() != 0)
        V8THROW_ERROR("Qt.fontFamilies(): Invalid arguments");

    QFontDatabase database;
    return V8ENGINE()->fromVariant(database.families());
}

/*!
\qmlmethod string Qt::md5(data)
Returns a hex string of the md5 hash of \c data.
*/
v8::Handle<v8::Value> QV8Engine::md5(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("Qt.md5(): Invalid arguments");

    QByteArray data = V8ENGINE()->toString(args[0]->ToString()).toUtf8();
    QByteArray result = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    return V8ENGINE()->toString(QLatin1String(result.toHex()));
}

/*!
\qmlmethod string Qt::btoa(data)
Binary to ASCII - this function returns a base64 encoding of \c data.
*/
v8::Handle<v8::Value> QV8Engine::btoa(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("Qt.btoa(): Invalid arguments");

    QByteArray data = V8ENGINE()->toString(args[0]->ToString()).toUtf8();

    return V8ENGINE()->toString(QLatin1String(data.toBase64()));
}

/*!
\qmlmethod string Qt::atob(data)
ASCII to binary - this function returns a base64 decoding of \c data.
*/
v8::Handle<v8::Value> QV8Engine::atob(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("Qt.atob(): Invalid arguments");

    QByteArray data = V8ENGINE()->toString(args[0]->ToString()).toUtf8();

    return V8ENGINE()->toString(QLatin1String(QByteArray::fromBase64(data)));
}

/*!
\qmlmethod Qt::quit()
This function causes the QDeclarativeEngine::quit() signal to be emitted.
Within the \l {QML Viewer}, this causes the launcher application to exit;
to quit a C++ application when this method is called, connect the
QDeclarativeEngine::quit() signal to the QCoreApplication::quit() slot.
*/
v8::Handle<v8::Value> QV8Engine::quit(const v8::Arguments &args)
{
    QDeclarativeEnginePrivate::get(V8ENGINE()->engine())->sendQuit();
    return v8::Undefined();
}

/*!
\qmlmethod object Qt::createQmlObject(string qml, object parent, string filepath)

Returns a new object created from the given \a string of QML which will have the specified \a parent,
or \c null if there was an error in creating the object.

If \a filepath is specified, it will be used for error reporting for the created object.

Example (where \c parentItem is the id of an existing QML item):

\snippet doc/src/snippets/declarative/createQmlObject.qml 0

In the case of an error, a QtScript Error object is thrown. This object has an additional property,
\c qmlErrors, which is an array of the errors encountered.
Each object in this array has the members \c lineNumber, \c columnNumber, \c fileName and \c message.
For example, if the above snippet had misspelled color as 'colro' then the array would contain an object like the following:
{ "lineNumber" : 1, "columnNumber" : 32, "fileName" : "dynamicSnippet1", "message" : "Cannot assign to non-existent property \"colro\""}.

Note that this function returns immediately, and therefore may not work if
the \a qml string loads new components (that is, external QML files that have not yet been loaded).
If this is the case, consider using \l{QML:Qt::createComponent()}{Qt.createComponent()} instead.

See \l {Dynamic Object Management in QML} for more information on using this function.
*/
v8::Handle<v8::Value> QV8Engine::createQmlObject(const v8::Arguments &args)
{
    if (args.Length() < 2 || args.Length() > 3) 
        V8THROW_ERROR("Qt.createQmlObject(): Invalid arguments");

    struct Error {
        static v8::Local<v8::Value> create(QV8Engine *engine, const QList<QDeclarativeError> &errors) {
            QString errorstr = QLatin1String("Qt.createQmlObject(): failed to create object: ");

            v8::Local<v8::Array> qmlerrors = v8::Array::New(errors.count());
            for (int ii = 0; ii < errors.count(); ++ii) {
                const QDeclarativeError &error = errors.at(ii);
                errorstr += QLatin1String("\n    ") + error.toString();
                v8::Local<v8::Object> qmlerror = v8::Object::New();
                qmlerror->Set(v8::String::New("lineNumber"), v8::Integer::New(error.line()));
                qmlerror->Set(v8::String::New("columnNumber"), v8::Integer::New(error.line()));
                qmlerror->Set(v8::String::New("fileName"), engine->toString(error.url().toString()));
                qmlerror->Set(v8::String::New("message"), engine->toString(error.description()));
                qmlerrors->Set(ii, qmlerror);
            }

            v8::Local<v8::Value> error = v8::Exception::Error(engine->toString(errorstr));
            v8::Local<v8::Object> errorObject = error->ToObject();
            errorObject->Set(v8::String::New("qmlErrors"), qmlerrors);
            return error;
        }
    };

    QV8Engine *v8engine = V8ENGINE();
    QDeclarativeEngine *engine = v8engine->engine();
    
    QDeclarativeContextData *context = v8engine->callingContext();
    QDeclarativeContext *effectiveContext = 0;
    if (context->isPragmaLibraryContext)
        effectiveContext = engine->rootContext();
    else
        effectiveContext = context->asQDeclarativeContext();
    Q_ASSERT(context && effectiveContext);

    QString qml = v8engine->toString(args[0]->ToString());
    if (qml.isEmpty())
        return v8::Null();

    QUrl url;
    if(args.Length() > 2)
        url = QUrl(v8engine->toString(args[2]->ToString()));
    else
        url = QUrl(QLatin1String("inline"));

    if (url.isValid() && url.isRelative())
        url = context->resolvedUrl(url);

    QObject *parentArg = v8engine->toQObject(args[1]);
    if(!parentArg)
        V8THROW_ERROR("Qt.createQmlObject(): Missing parent object");

    QDeclarativeComponent component(engine);
    component.setData(qml.toUtf8(), url);

    if(component.isError()) {
        v8::ThrowException(Error::create(v8engine, component.errors()));
        return v8::Undefined();
    }

    if (!component.isReady())
        V8THROW_ERROR("Qt.createQmlObject(): Component is not ready");

    QObject *obj = component.beginCreate(effectiveContext);
    if(obj)
        QDeclarativeData::get(obj, true)->setImplicitDestructible();
    component.completeCreate();

    if(component.isError()) {
        v8::ThrowException(Error::create(v8engine, component.errors()));
        return v8::Undefined();
    }

    Q_ASSERT(obj);

    obj->setParent(parentArg);

    QList<QDeclarativePrivate::AutoParentFunction> functions = QDeclarativeMetaType::parentFunctions();
    for (int ii = 0; ii < functions.count(); ++ii) {
        if (QDeclarativePrivate::Parented == functions.at(ii)(obj, parentArg))
            break;
    }

    return v8engine->newQObject(obj);
}

/*!
\qmlmethod object Qt::createComponent(url)

Returns a \l Component object created using the QML file at the specified \a url,
or \c null if an empty string was given.

The returned component's \l Component::status property indicates whether the
component was successfully created. If the status is \c Component.Error, 
see \l Component::errorString() for an error description.

Call \l {Component::createObject()}{Component.createObject()} on the returned
component to create an object instance of the component.

For example:

\snippet doc/src/snippets/declarative/createComponent-simple.qml 0

See \l {Dynamic Object Management in QML} for more information on using this function.

To create a QML object from an arbitrary string of QML (instead of a file),
use \l{QML:Qt::createQmlObject()}{Qt.createQmlObject()}.
*/
v8::Handle<v8::Value> QV8Engine::createComponent(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("Qt.createComponent(): Invalid arguments");

    QV8Engine *v8engine = V8ENGINE();
    QDeclarativeEngine *engine = v8engine->engine();

    QDeclarativeContextData *context = v8engine->callingContext();
    QDeclarativeContextData *effectiveContext = context;
    if (context->isPragmaLibraryContext)
        effectiveContext = 0;
    Q_ASSERT(context);

    QString arg = v8engine->toString(args[0]->ToString());
    if (arg.isEmpty())
        return v8::Null();

    QUrl url = context->resolvedUrl(QUrl(arg));
    QDeclarativeComponent *c = new QDeclarativeComponent(engine, url, engine);
    QDeclarativeComponentPrivate::get(c)->creationContext = effectiveContext;
    QDeclarativeData::get(c, true)->setImplicitDestructible();
    return v8engine->newQObject(c);
}

v8::Handle<v8::Value> QV8Engine::qsTranslate(const v8::Arguments &args)
{
    if (args.Length() < 2)
        V8THROW_ERROR("qsTranslate() requires at least two arguments");
    if (!args[0]->IsString())
        V8THROW_ERROR("qsTranslate(): first argument (context) must be a string");
    if (!args[1]->IsString())
        V8THROW_ERROR("qsTranslate(): second argument (text) must be a string");
    if ((args.Length() > 2) && !args[2]->IsString())
        V8THROW_ERROR("qsTranslate(): third argument (comment) must be a string");
    if ((args.Length() > 3) && !args[3]->IsString())
        V8THROW_ERROR("qsTranslate(): fourth argument (encoding) must be a string");

    QV8Engine *v8engine = V8ENGINE();
    QString context = v8engine->toString(args[0]);
    QString text = v8engine->toString(args[1]);
    QString comment;
    if (args.Length() > 2) comment = v8engine->toString(args[2]);

    QCoreApplication::Encoding encoding = QCoreApplication::UnicodeUTF8;
    if (args.Length() > 3) {
        QString encStr = v8engine->toString(args[3]);
        if (encStr == QLatin1String("CodecForTr")) {
            encoding = QCoreApplication::CodecForTr;
        } else if (encStr == QLatin1String("UnicodeUTF8")) {
            encoding = QCoreApplication::UnicodeUTF8;
        } else {
            QString msg = QString::fromLatin1("qsTranslate(): invalid encoding '%0'").arg(encStr);
            V8THROW_ERROR((uint16_t *)msg.constData());
        }
    }

    int n = -1;
    if (args.Length() > 4)
        n = args[4]->Int32Value();

    QString result = QCoreApplication::translate(context.toUtf8().constData(),
                                                 text.toUtf8().constData(),
                                                 comment.toUtf8().constData(),
                                                 encoding, n);

    return v8engine->toString(result);
}

v8::Handle<v8::Value> QV8Engine::qsTranslateNoOp(const v8::Arguments &args)
{
    if (args.Length() < 2)
        return v8::Undefined();
    return args[1];
}

v8::Handle<v8::Value> QV8Engine::qsTr(const v8::Arguments &args)
{
    if (args.Length() < 1)
        V8THROW_ERROR("qsTr() requires at least one argument");
    if (!args[0]->IsString())
        V8THROW_ERROR("qsTr(): first argument (text) must be a string");
    if ((args.Length() > 1) && !args[1]->IsString())
        V8THROW_ERROR("qsTr(): second argument (comment) must be a string");
    if ((args.Length() > 2) && !args[2]->IsNumber())
        V8THROW_ERROR("qsTr(): third argument (n) must be a number");

    QV8Engine *v8engine = V8ENGINE();
    QDeclarativeContextData *ctxt = v8engine->callingContext();

    QString path = ctxt->url.toString();
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    QString context = (lastSlash > -1) ? path.mid(lastSlash + 1, path.length()-lastSlash-5) : QString();

    QString text = v8engine->toString(args[0]);
    QString comment;
    if (args.Length() > 1)
        comment = v8engine->toString(args[1]);
    int n = -1;
    if (args.Length() > 2)
        n = args[2]->Int32Value();

    QString result = QCoreApplication::translate(context.toUtf8().constData(), text.toUtf8().constData(),
                                                 comment.toUtf8().constData(), QCoreApplication::UnicodeUTF8, n);

    return v8engine->toString(result);
}

v8::Handle<v8::Value> QV8Engine::qsTrNoOp(const v8::Arguments &args)
{
    if (args.Length() < 1)
        return v8::Undefined();
    return args[0];
}

v8::Handle<v8::Value> QV8Engine::qsTrId(const v8::Arguments &args)
{
    if (args.Length() < 1)
        V8THROW_ERROR("qsTrId() requires at least one argument");
    if (!args[0]->IsString())
        V8THROW_TYPE("qsTrId(): first argument (id) must be a string");
    if (args.Length() > 1 && !args[1]->IsNumber())
        V8THROW_TYPE("qsTrId(): second argument (n) must be a number");

    int n = -1;
    if (args.Length() > 1)
        n = args[1]->Int32Value();

    QV8Engine *v8engine = V8ENGINE();
    return v8engine->toString(qtTrId(v8engine->toString(args[0]).toUtf8().constData(), n));
}

v8::Handle<v8::Value> QV8Engine::qsTrIdNoOp(const v8::Arguments &args)
{
    if (args.Length() < 1)
        return v8::Undefined();
    return args[0];
}

void QV8Engine::initDeclarativeGlobalObject()
{
    v8::HandleScope handels;
    v8::Context::Scope contextScope(m_context);
    initializeGlobal(m_context->Global());
    freezeObject(m_context->Global());
}

void QV8Engine::setEngine(QDeclarativeEngine *engine)
{
    m_engine = engine;
    initDeclarativeGlobalObject();
}

void QV8Engine::setException(v8::Handle<v8::Value> value, v8::Handle<v8::Message> msg)
{
    m_exception.set(value, msg);
}

v8::Handle<v8::Value> QV8Engine::throwException(v8::Handle<v8::Value> value)
{
    setException(value);
    v8::ThrowException(value);
    return value;
}

void QV8Engine::clearExceptions()
{
    m_exception.clear();
}

v8::Handle<v8::Value> QV8Engine::uncaughtException() const
{
    if (!hasUncaughtException())
        return v8::Handle<v8::Value>();
    return m_exception;
}

bool QV8Engine::hasUncaughtException() const
{
    return m_exception;
}

int QV8Engine::uncaughtExceptionLineNumber() const
{
    return m_exception.lineNumber();
}

QStringList QV8Engine::uncaughtExceptionBacktrace() const
{
    return m_exception.backtrace();
}

/*!
  \internal
  Save the current exception on stack so it can be set again later.
  \sa QV8Engine::restoreException
*/
void QV8Engine::saveException()
{
    m_exception.push();
}

/*!
  \internal
  Load a saved exception from stack. Current exception, if exists will be dropped
  \sa QV8Engine::saveException
*/
void QV8Engine::restoreException()
{
    m_exception.pop();
}

QV8Engine::Exception::Exception() {}

QV8Engine::Exception::~Exception()
{
    Q_ASSERT_X(m_stack.isEmpty(), Q_FUNC_INFO, "Some saved exceptions left. Asymetric pop/push found.");
    clear();
}

void QV8Engine::Exception::set(v8::Handle<v8::Value> value, v8::Handle<v8::Message> message)
{
    Q_ASSERT_X(!value.IsEmpty(), Q_FUNC_INFO, "Throwing an empty value handle is highly suspected");
    clear();
    m_value = v8::Persistent<v8::Value>::New(value);
    m_message = v8::Persistent<v8::Message>::New(message);
}

void QV8Engine::Exception::clear()
{
    m_value.Dispose();
    m_value.Clear();
    m_message.Dispose();
    m_message.Clear();
}

QV8Engine::Exception::operator bool() const
{
    return !m_value.IsEmpty();
}

QV8Engine::Exception::operator v8::Handle<v8::Value>() const
{
    Q_ASSERT(*this);
    return m_value;
}

int QV8Engine::Exception::lineNumber() const
{
    if (m_message.IsEmpty())
        return -1;
    return m_message->GetLineNumber();
}

QStringList QV8Engine::Exception::backtrace() const
{
    if (m_message.IsEmpty())
        return QStringList();

    QStringList backtrace;
    v8::Handle<v8::StackTrace> trace = m_message->GetStackTrace();
    if (trace.IsEmpty())
        // FIXME it should not happen (SetCaptureStackTraceForUncaughtExceptions is called).
        return QStringList();

    for (int i = 0; i < trace->GetFrameCount(); ++i) {
        v8::Local<v8::StackFrame> frame = trace->GetFrame(i);
        backtrace.append(QJSConverter::toString(frame->GetFunctionName()));
        backtrace.append(QJSConverter::toString(frame->GetFunctionName()));
        backtrace.append(QString::fromAscii("()@"));
        backtrace.append(QJSConverter::toString(frame->GetScriptName()));
        backtrace.append(QString::fromAscii(":"));
        backtrace.append(QString::number(frame->GetLineNumber()));
    }
    return backtrace;
}

void QV8Engine::Exception::push()
{
    m_stack.push(qMakePair(m_value, m_message));
    m_value.Clear();
    m_message.Clear();
}

void QV8Engine::Exception::pop()
{
    Q_ASSERT_X(!m_stack.empty(), Q_FUNC_INFO, "Attempt to load unsaved exception found");
    ValueMessagePair pair = m_stack.pop();
    clear();
    m_value = pair.first;
    m_message = pair.second;
}

QScriptPassPointer<QJSValuePrivate> QV8Engine::newRegExp(const QString &pattern, const QString &flags)
{
    int f = v8::RegExp::kNone;

    QString::const_iterator i = flags.constBegin();
    for (; i != flags.constEnd(); ++i) {
        switch (i->unicode()) {
        case 'i':
            f |= v8::RegExp::kIgnoreCase;
            break;
        case 'm':
            f |= v8::RegExp::kMultiline;
            break;
        case 'g':
            f |= v8::RegExp::kGlobal;
            break;
        default:
            {
                // ignore a Syntax Error.
            }
        }
    }

    v8::Handle<v8::RegExp> regexp = v8::RegExp::New(QJSConverter::toString(pattern), static_cast<v8::RegExp::Flags>(f));
    return new QJSValuePrivate(this, regexp);
}

QScriptPassPointer<QJSValuePrivate> QV8Engine::newRegExp(const QRegExp &regexp)
{
    return new QJSValuePrivate(this, QJSConverter::toRegExp(regexp));
}


// Converts a QVariantList to JS.
// The result is a new Array object with length equal to the length
// of the QVariantList, and the elements being the QVariantList's
// elements converted to JS, recursively.
v8::Handle<v8::Array> QV8Engine::variantListToJS(const QVariantList &lst)
{
    v8::Handle<v8::Array> result = v8::Array::New(lst.size());
    for (int i = 0; i < lst.size(); ++i)
        result->Set(i, variantToJS(lst.at(i)));
    return result;
}

// Converts a JS Array object to a QVariantList.
// The result is a QVariantList with length equal to the length
// of the JS Array, and elements being the JS Array's elements
// converted to QVariants, recursively.
QVariantList QV8Engine::variantListFromJS(v8::Handle<v8::Array> jsArray)
{
    QVariantList result;
    int hash = jsArray->GetIdentityHash();
    if (visitedConversionObjects.contains(hash))
        return result; // Avoid recursion.
    v8::HandleScope handleScope;
    visitedConversionObjects.insert(hash);
    uint32_t length = jsArray->Length();
    for (uint32_t i = 0; i < length; ++i)
        result.append(variantFromJS(jsArray->Get(i)));
    visitedConversionObjects.remove(hash);
    return result;
}

// Converts a QVariantMap to JS.
// The result is a new Object object with property names being
// the keys of the QVariantMap, and values being the values of
// the QVariantMap converted to JS, recursively.
v8::Handle<v8::Object> QV8Engine::variantMapToJS(const QVariantMap &vmap)
{
    v8::Handle<v8::Object> result = v8::Object::New();
    QVariantMap::const_iterator it;
    for (it = vmap.constBegin(); it != vmap.constEnd(); ++it)
        result->Set(QJSConverter::toString(it.key()), variantToJS(it.value()));
    return result;
}

// Converts a JS Object to a QVariantMap.
// The result is a QVariantMap with keys being the property names
// of the object, and values being the values of the JS object's
// properties converted to QVariants, recursively.
QVariantMap QV8Engine::variantMapFromJS(v8::Handle<v8::Object> jsObject)
{
    QVariantMap result;
    int hash = jsObject->GetIdentityHash();
    if (visitedConversionObjects.contains(hash))
        return result; // Avoid recursion.
    visitedConversionObjects.insert(hash);
    v8::HandleScope handleScope;
    // TODO: Only object's own property names. Include non-enumerable properties.
    v8::Handle<v8::Array> propertyNames = jsObject->GetPropertyNames();
    uint32_t length = propertyNames->Length();
    for (uint32_t i = 0; i < length; ++i) {
        v8::Handle<v8::Value> name = propertyNames->Get(i);
        result.insert(QJSConverter::toString(name->ToString()), variantFromJS(jsObject->Get(name)));
    }
    visitedConversionObjects.remove(hash);
    return result;
}

// Converts the meta-type defined by the given type and data to JS.
// Returns the value if conversion succeeded, an empty handle otherwise.
v8::Handle<v8::Value> QV8Engine::metaTypeToJS(int type, const void *data)
{
    Q_ASSERT(data != 0);
    v8::Handle<v8::Value> result;

    // check if it's one of the types we know
    switch (QMetaType::Type(type)) {
    case QMetaType::Void:
        return v8::Undefined();
    case QMetaType::Bool:
        return v8::Boolean::New(*reinterpret_cast<const bool*>(data));
    case QMetaType::Int:
        return v8::Int32::New(*reinterpret_cast<const int*>(data));
    case QMetaType::UInt:
        return v8::Uint32::New(*reinterpret_cast<const uint*>(data));
    case QMetaType::LongLong:
        return v8::Number::New(double(*reinterpret_cast<const qlonglong*>(data)));
    case QMetaType::ULongLong:
#if defined(Q_OS_WIN) && defined(_MSC_FULL_VER) && _MSC_FULL_VER <= 12008804
#pragma message("** NOTE: You need the Visual Studio Processor Pack to compile support for 64bit unsigned integers.")
        return v8::Number::New(double((qlonglong)*reinterpret_cast<const qulonglong*>(data)));
#elif defined(Q_CC_MSVC) && !defined(Q_CC_MSVC_NET)
        return v8::Number::New(double((qlonglong)*reinterpret_cast<const qulonglong*>(data)));
#else
        return v8::Number::New(double(*reinterpret_cast<const qulonglong*>(data)));
#endif
    case QMetaType::Double:
        return v8::Number::New(double(*reinterpret_cast<const double*>(data)));
    case QMetaType::QString:
        return QJSConverter::toString(*reinterpret_cast<const QString*>(data));
    case QMetaType::Float:
        return v8::Number::New(*reinterpret_cast<const float*>(data));
    case QMetaType::Short:
        return v8::Int32::New(*reinterpret_cast<const short*>(data));
    case QMetaType::UShort:
        return v8::Uint32::New(*reinterpret_cast<const unsigned short*>(data));
    case QMetaType::Char:
        return v8::Int32::New(*reinterpret_cast<const char*>(data));
    case QMetaType::UChar:
        return v8::Uint32::New(*reinterpret_cast<const unsigned char*>(data));
    case QMetaType::QChar:
        return v8::Uint32::New((*reinterpret_cast<const QChar*>(data)).unicode());
    case QMetaType::QStringList:
        result = QJSConverter::toStringList(*reinterpret_cast<const QStringList *>(data));
        break;
    case QMetaType::QVariantList:
        result = variantListToJS(*reinterpret_cast<const QVariantList *>(data));
        break;
    case QMetaType::QVariantMap:
        result = variantMapToJS(*reinterpret_cast<const QVariantMap *>(data));
        break;
    case QMetaType::QDateTime:
        result = QJSConverter::toDateTime(*reinterpret_cast<const QDateTime *>(data));
        break;
    case QMetaType::QDate:
        result = QJSConverter::toDateTime(QDateTime(*reinterpret_cast<const QDate *>(data)));
        break;
    case QMetaType::QRegExp:
        result = QJSConverter::toRegExp(*reinterpret_cast<const QRegExp *>(data));
        break;
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
        result = newQObject(*reinterpret_cast<QObject* const *>(data));
        break;
    case QMetaType::QVariant:
        result = variantToJS(*reinterpret_cast<const QVariant*>(data));
        break;
    default:
        if (type == qMetaTypeId<QJSValue>()) {
            return QJSValuePrivate::get(*reinterpret_cast<const QJSValue*>(data))->asV8Value(this);
        } else {
            QByteArray typeName = QMetaType::typeName(type);
            if (typeName.endsWith('*') && !*reinterpret_cast<void* const *>(data)) {
                return v8::Null();
            } else {
                // Fall back to wrapping in a QVariant.
                result = newVariant(QVariant(type, data));
            }
        }
    }
    return result;
}

// Converts a JS value to a meta-type.
// data must point to a place that can store a value of the given type.
// Returns true if conversion succeeded, false otherwise.
bool QV8Engine::metaTypeFromJS(v8::Handle<v8::Value> value, int type, void *data) {
    // check if it's one of the types we know
    switch (QMetaType::Type(type)) {
    case QMetaType::Bool:
        *reinterpret_cast<bool*>(data) = value->ToBoolean()->Value();
        return true;
    case QMetaType::Int:
        *reinterpret_cast<int*>(data) = value->ToInt32()->Value();
        return true;
    case QMetaType::UInt:
        *reinterpret_cast<uint*>(data) = value->ToUint32()->Value();
        return true;
    case QMetaType::LongLong:
        *reinterpret_cast<qlonglong*>(data) = qlonglong(value->ToInteger()->Value());
        return true;
    case QMetaType::ULongLong:
        *reinterpret_cast<qulonglong*>(data) = qulonglong(value->ToInteger()->Value());
        return true;
    case QMetaType::Double:
        *reinterpret_cast<double*>(data) = value->ToNumber()->Value();
        return true;
    case QMetaType::QString:
        if (value->IsUndefined() || value->IsNull())
            *reinterpret_cast<QString*>(data) = QString();
        else
            *reinterpret_cast<QString*>(data) = QJSConverter::toString(value->ToString());
        return true;
    case QMetaType::Float:
        *reinterpret_cast<float*>(data) = value->ToNumber()->Value();
        return true;
    case QMetaType::Short:
        *reinterpret_cast<short*>(data) = short(value->ToInt32()->Value());
        return true;
    case QMetaType::UShort:
        *reinterpret_cast<unsigned short*>(data) = ushort(value->ToInt32()->Value()); // ### QScript::ToUInt16()
        return true;
    case QMetaType::Char:
        *reinterpret_cast<char*>(data) = char(value->ToInt32()->Value());
        return true;
    case QMetaType::UChar:
        *reinterpret_cast<unsigned char*>(data) = (unsigned char)(value->ToInt32()->Value());
        return true;
    case QMetaType::QChar:
        if (value->IsString()) {
            QString str = QJSConverter::toString(v8::Handle<v8::String>::Cast(value));
            *reinterpret_cast<QChar*>(data) = str.isEmpty() ? QChar() : str.at(0);
        } else {
            *reinterpret_cast<QChar*>(data) = QChar(ushort(value->ToInt32()->Value())); // ### QScript::ToUInt16()
        }
        return true;
    case QMetaType::QDateTime:
        if (value->IsDate()) {
            *reinterpret_cast<QDateTime *>(data) = QJSConverter::toDateTime(v8::Handle<v8::Date>::Cast(value));
            return true;
        } break;
    case QMetaType::QDate:
        if (value->IsDate()) {
            *reinterpret_cast<QDate *>(data) = QJSConverter::toDateTime(v8::Handle<v8::Date>::Cast(value)).date();
            return true;
        } break;
    case QMetaType::QRegExp:
        if (value->IsRegExp()) {
            *reinterpret_cast<QRegExp *>(data) = QJSConverter::toRegExp(v8::Handle<v8::RegExp>::Cast(value));
            return true;
        } break;
    case QMetaType::QObjectStar:
        if (isQObject(value) || value->IsNull()) {
            *reinterpret_cast<QObject* *>(data) = qtObjectFromJS(value);
            return true;
        } break;
    case QMetaType::QWidgetStar:
        if (isQObject(value) || value->IsNull()) {
            QObject *qo = qtObjectFromJS(value);
            if (!qo || qo->isWidgetType()) {
                *reinterpret_cast<QWidget* *>(data) = reinterpret_cast<QWidget*>(qo);
                return true;
            }
        } break;
    case QMetaType::QStringList:
        if (value->IsArray()) {
            *reinterpret_cast<QStringList *>(data) = QJSConverter::toStringList(v8::Handle<v8::Array>::Cast(value));
            return true;
        } break;
    case QMetaType::QVariantList:
        if (value->IsArray()) {
            *reinterpret_cast<QVariantList *>(data) = variantListFromJS(v8::Handle<v8::Array>::Cast(value));
            return true;
        } break;
    case QMetaType::QVariantMap:
        if (value->IsObject()) {
            *reinterpret_cast<QVariantMap *>(data) = variantMapFromJS(v8::Handle<v8::Object>::Cast(value));
            return true;
        } break;
    case QMetaType::QVariant:
        *reinterpret_cast<QVariant*>(data) = variantFromJS(value);
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
        if (var.canConvert(QVariant::Type(type))) {
            QVariant vv = var;
            vv.convert(QVariant::Type(type));
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
    if (isVariant(value) && name.endsWith('*')) {
        int valueType = QMetaType::type(name.left(name.size()-1));
        QVariant &var = variantValue(value);
        if (valueType == var.userType()) {
            // We have T t, T* is requested, so return &t.
            *reinterpret_cast<void* *>(data) = var.data();
            return true;
        } else {
            // Look in the prototype chain.
            v8::Handle<v8::Value> proto = value->ToObject()->GetPrototype();
            while (proto->IsObject()) {
                bool canCast = false;
                if (isVariant(proto)) {
                    canCast = (type == variantValue(proto).userType())
                              || (valueType && (valueType == variantValue(proto).userType()));
                }
                else if (isQObject(proto)) {
                    QByteArray className = name.left(name.size()-1);
                    if (QObject *qobject = qtObjectFromJS(proto))
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
                proto = proto->ToObject()->GetPrototype();
            }
        }
    } else if (value->IsNull() && name.endsWith('*')) {
        *reinterpret_cast<void* *>(data) = 0;
        return true;
    } else if (type == qMetaTypeId<QJSValue>()) {
        *reinterpret_cast<QJSValue*>(data) = QJSValuePrivate::get(new QJSValuePrivate(this, value));
        return true;
    }

    return false;
}

// Converts a QVariant to JS.
v8::Handle<v8::Value> QV8Engine::variantToJS(const QVariant &value)
{
    return metaTypeToJS(value.userType(), value.constData());
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
QVariant QV8Engine::variantFromJS(v8::Handle<v8::Value> value)
{
    Q_ASSERT(!value.IsEmpty());
    if (value->IsNull() || value->IsUndefined())
        return QVariant();
    if (value->IsBoolean())
        return value->ToBoolean()->Value();
    if (value->IsInt32())
        return value->ToInt32()->Value();
    if (value->IsNumber())
        return value->ToNumber()->Value();
    if (value->IsString())
        return QJSConverter::toString(value->ToString());
    Q_ASSERT(value->IsObject());
    if (value->IsArray())
        return variantListFromJS(v8::Handle<v8::Array>::Cast(value));
    if (value->IsDate())
        return QJSConverter::toDateTime(v8::Handle<v8::Date>::Cast(value));
    if (value->IsRegExp())
        return QJSConverter::toRegExp(v8::Handle<v8::RegExp>::Cast(value));
    if (isVariant(value))
        return variantValue(value);
    if (isQObject(value))
        return qVariantFromValue(qtObjectFromJS(value));
    return variantMapFromJS(value->ToObject());
}

bool QV8Engine::convertToNativeQObject(v8::Handle<v8::Value> value,
                                                  const QByteArray &targetType,
                                                  void **result)
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

QObject *QV8Engine::qtObjectFromJS(v8::Handle<v8::Value> value)
{
    if (!value->IsObject())
        return 0;

    QV8ObjectResource *r = (QV8ObjectResource *)value->ToObject()->GetExternalResource();
    if (!r)
        return 0;
    QV8ObjectResource::ResourceType type = r->resourceType();
    if (type == QV8ObjectResource::QObjectType)
        return qobjectWrapper()->toQObject(r);
    else if (type == QV8ObjectResource::VariantType) {
        QVariant variant = variantWrapper()->toVariant(r);
        int type = variant.userType();
        if ((type == QMetaType::QObjectStar) || (type == QMetaType::QWidgetStar))
            return *reinterpret_cast<QObject* const *>(variant.constData());
    }
    return 0;
}


QVariant &QV8Engine::variantValue(v8::Handle<v8::Value> value)
{
    return variantWrapper()->variantValue(value);
}

// Creates a QVariant wrapper object.
v8::Handle<v8::Object> QV8Engine::newVariant(const QVariant &value)
{
    v8::Handle<v8::Object> instance = variantWrapper()->newVariant(value);
    return instance;
}

QScriptPassPointer<QJSValuePrivate> QV8Engine::evaluate(v8::Handle<v8::Script> script, v8::TryCatch& tryCatch)
{
    v8::HandleScope handleScope;

    clearExceptions();
    if (script.IsEmpty()) {
        v8::Handle<v8::Value> exception = tryCatch.Exception();
        if (exception.IsEmpty()) {
            // This is possible on syntax errors like { a:12, b:21 } <- missing "(", ")" around expression.
            return InvalidValue();
        }
        setException(exception, tryCatch.Message());
        return new QJSValuePrivate(this, exception);
    }
    v8::Handle<v8::Value> result;
    result = script->Run();
    if (result.IsEmpty()) {
        v8::Handle<v8::Value> exception = tryCatch.Exception();
        // TODO: figure out why v8 doesn't always produce an exception value
        //Q_ASSERT(!exception.IsEmpty());
        if (exception.IsEmpty())
            exception = v8::Exception::Error(v8::String::New("missing exception value"));
        setException(exception, tryCatch.Message());
        return new QJSValuePrivate(this, exception);
    }
    return new QJSValuePrivate(this, result);
}

QJSValue QV8Engine::scriptValueFromInternal(v8::Handle<v8::Value> value) const
{
    if (value.IsEmpty())
        return QJSValuePrivate::get(InvalidValue());
    return QJSValuePrivate::get(new QJSValuePrivate(const_cast<QV8Engine*>(this), value));
}

QScriptPassPointer<QJSValuePrivate> QV8Engine::newArray(uint length)
{
    return new QJSValuePrivate(this, v8::Array::New(length));
}

void QV8Engine::emitSignalHandlerException()
{
    emit q->signalHandlerException(scriptValueFromInternal(uncaughtException()));
}

QT_END_NAMESPACE

