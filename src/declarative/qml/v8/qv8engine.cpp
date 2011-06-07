/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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
#include "qv8include_p.h"
#include "../../../3rdparty/javascriptcore/DateMath.h"

#include <private/qdeclarativelist_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativecomponent_p.h>
#include <private/qdeclarativestringconverters_p.h>

#include <QtDeclarative/qdeclarativecomponent.h>

#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qnumeric.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qfontdatabase.h>
#include <private/qdeclarativeapplication_p.h>
#include <private/qdeclarativexmlhttprequest_p.h>
#include <private/qdeclarativesqldatabase_p.h>

// XXX Need to check all the global functions will also work in a worker script where the QDeclarativeEngine
// is not available
QT_BEGIN_NAMESPACE

QV8Engine::QV8Engine()
: m_xmlHttpRequestData(0), m_sqlDatabaseData(0), m_listModelData(0)
{
}

QV8Engine::~QV8Engine()
{
    qt_rem_qmlsqldatabase(this, m_sqlDatabaseData); 
    m_sqlDatabaseData = 0;
    qt_rem_qmlxmlhttprequest(this, m_xmlHttpRequestData); 
    m_xmlHttpRequestData = 0;
    delete m_listModelData;
    m_listModelData = 0;

    m_getOwnPropertyNames.Dispose(); m_getOwnPropertyNames.Clear();
    
    m_valueTypeWrapper.destroy();
    m_variantWrapper.destroy();
    m_listWrapper.destroy();
    m_typeWrapper.destroy();
    m_qobjectWrapper.destroy();
    m_contextWrapper.destroy();
    m_stringWrapper.destroy();
    m_context.Dispose();
    m_context.Clear();
}

void QV8Engine::init(QDeclarativeEngine *engine)
{
    m_engine = engine;

    QByteArray v8args = qgetenv("V8ARGS");
    if (!v8args.isEmpty())
        v8::V8::SetFlagsFromString(v8args.constData(), v8args.length());

    v8::HandleScope handle_scope;
    m_context = v8::Context::New();
    v8::Context::Scope context_scope(m_context);

    m_stringWrapper.init();
    m_contextWrapper.init(this);
    m_qobjectWrapper.init(this);
    m_typeWrapper.init(this);
    m_listWrapper.init(this);
    m_variantWrapper.init(this);
    m_valueTypeWrapper.init(this);

    {
    v8::Handle<v8::Value> v = global()->Get(v8::String::New("Object"))->ToObject()->Get(v8::String::New("getOwnPropertyNames"));
    m_getOwnPropertyNames = v8::Persistent<v8::Function>::New(v8::Handle<v8::Function>::Cast(v));
    }

    initializeGlobal(m_context->Global());
    freezeGlobal();
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

// Converts a QRegExp to a JS RegExp.
// The conversion is not 100% exact since ECMA regexp and QRegExp
// have different semantics/flags, but we try to do our best.
static v8::Handle<v8::RegExp> regexpFromQRegExp(QV8Engine *engine, const QRegExp &re)
{
    // Convert the pattern to a ECMAScript pattern.
    QString pattern = qt_regexp_toCanonical(re.pattern(), re.patternSyntax());
    if (re.isMinimal()) {
        QString ecmaPattern;
        int len = pattern.length();
        ecmaPattern.reserve(len);
        int i = 0;
        const QChar *wc = pattern.unicode();
        bool inBracket = false;
        while (i < len) {
            QChar c = wc[i++];
            ecmaPattern += c;
            switch (c.unicode()) {
            case '?':
            case '+':
            case '*':
            case '}':
                if (!inBracket)
                    ecmaPattern += QLatin1Char('?');
                break;
            case '\\':
                if (i < len)
                    ecmaPattern += wc[i++];
                break;
            case '[':
                inBracket = true;
                break;
            case ']':
                inBracket = false;
                break;
            default:
                break;
            }
        }
        pattern = ecmaPattern;
    }

    int flags = v8::RegExp::kNone;
    if (re.caseSensitivity() == Qt::CaseInsensitive)
        flags |= v8::RegExp::kIgnoreCase;

    return v8::RegExp::New(engine->toString(pattern), static_cast<v8::RegExp::Flags>(flags));
}

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
                return regexpFromQRegExp(this, *reinterpret_cast<const QRegExp *>(ptr));
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

        if (QDeclarativeValueType *vt = QDeclarativeEnginePrivate::get(m_engine)->valueTypes[type]) 
            return m_valueTypeWrapper.newValueType(variant, vt);

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
            // XXX aakenned Can this be more optimal?  Just use Array as a prototype and 
            // implement directly against QList<QObject*>?
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

    return m_variantWrapper.newVariant(variant);

    // XXX aakenned
#if 0
#ifndef QT_NO_REGEXP
        case QMetaType::QRegExp:
            result = newRegExp(exec, *reinterpret_cast<const QRegExp *>(ptr));
            break;
#endif
#ifndef QT_NO_QOBJECT
#endif
        case QMetaType::QVariant:
            result = eng->newVariant(*reinterpret_cast<const QVariant*>(ptr));
            break;
        default:
            if (type == qMetaTypeId<QScriptValue>()) {
                result = eng->scriptValueToJSCValue(*reinterpret_cast<const QScriptValue*>(ptr));
                if (!result)
                    return JSC::jsUndefined();
            }

#ifndef QT_NO_QOBJECT
            // lazy registration of some common list types
            else if (type == qMetaTypeId<QObjectList>()) {
                qScriptRegisterSequenceMetaType<QObjectList>(eng->q_func());
                return create(exec, type, ptr);
            }
#endif
            else if (type == qMetaTypeId<QList<int> >()) {
                qScriptRegisterSequenceMetaType<QList<int> >(eng->q_func());
                return create(exec, type, ptr);
            }

            else {
                QByteArray typeName = QMetaType::typeName(type);
                if (typeName.endsWith('*') && !*reinterpret_cast<void* const *>(ptr))
                    return JSC::jsNull();
                else
                    result = eng->newVariant(QVariant(type, ptr));
            }
        }
    }
#endif
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
    else if (value->IsBoolean())
        return value->ToBoolean()->Value();
    else if (value->IsInt32())
        return value->ToInt32()->Value();
    else if (value->IsNumber())
        return value->ToNumber()->Value();
    else if (value->IsString())
        return m_stringWrapper.toString(value->ToString());
    if (value->IsDate())
        return qtDateTimeFromJsDate(v8::Handle<v8::Date>::Cast(value)->NumberValue());
    // NOTE: since we convert QTime to JS Date, round trip will change the variant type (to QDateTime)!

    Q_ASSERT(value->IsObject());

    if (value->IsRegExp()) {
        v8::Context::Scope scope(context());
        v8::Handle<v8::RegExp> jsRegExp = v8::Handle<v8::RegExp>::Cast(value);
        // Copied from QtScript
        // Converts a JS RegExp to a QRegExp.
        // The conversion is not 100% exact since ECMA regexp and QRegExp
        // have different semantics/flags, but we try to do our best.
        QString pattern = toString(jsRegExp->GetSource());
        Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
        if (jsRegExp->GetFlags() & v8::RegExp::kIgnoreCase)
            caseSensitivity = Qt::CaseInsensitive;
        return QRegExp(pattern, caseSensitivity, QRegExp::RegExp2);
    } else if (value->IsArray()) {
        v8::Context::Scope scope(context());
        QVariantList rv;

        v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(value);
        int length = array->Length();
        for (int ii = 0; ii < length; ++ii)
            rv << toVariant(array->Get(ii), -1);

        return rv;
    } else if (!value->IsFunction()) {
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

    // XXX - Qt global object properties

    v8::Local<v8::Object> qt = v8::Object::New();

    // Set all the enums from the "Qt" namespace
    const QMetaObject *qtMetaObject = StaticQtMetaObject::get();
    for (int ii = 0; ii < qtMetaObject->enumeratorCount(); ++ii) {
        QMetaEnum enumerator = qtMetaObject->enumerator(ii);
        for (int jj = 0; jj < enumerator.keyCount(); ++jj) {
            qt->Set(v8::String::New(enumerator.key(jj)), v8::Integer::New(enumerator.value(jj)));
        }
    }

    if (m_engine)
        qt->Set(v8::String::New("application"), newQObject(new QDeclarativeApplication(m_engine)));

    qt->Set(v8::String::New("include"), V8FUNCTION(QV8Include::include, this));
    qt->Set(v8::String::New("isQtObject"), V8FUNCTION(isQtObject, this));
    qt->Set(v8::String::New("rgba"), V8FUNCTION(rgba, this));
    qt->Set(v8::String::New("hsla"), V8FUNCTION(hsla, this));
    qt->Set(v8::String::New("rect"), V8FUNCTION(rect, this));
    qt->Set(v8::String::New("point"), V8FUNCTION(point, this));
    qt->Set(v8::String::New("size"), V8FUNCTION(size, this));
    qt->Set(v8::String::New("vector3d"), V8FUNCTION(vector3d, this));

    if (m_engine) {
        qt->Set(v8::String::New("lighter"), V8FUNCTION(lighter, this));
        qt->Set(v8::String::New("darker"), V8FUNCTION(darker, this));
        qt->Set(v8::String::New("tint"), V8FUNCTION(tint, this));
    }

    qt->Set(v8::String::New("formatDate"), V8FUNCTION(formatDate, this));
    qt->Set(v8::String::New("formatTime"), V8FUNCTION(formatTime, this));
    qt->Set(v8::String::New("formatDateTime"), V8FUNCTION(formatDateTime, this));

    qt->Set(v8::String::New("openUrlExternally"), V8FUNCTION(openUrlExternally, this));
    qt->Set(v8::String::New("fontFamilies"), V8FUNCTION(fontFamilies, this));
    qt->Set(v8::String::New("md5"), V8FUNCTION(md5, this));
    qt->Set(v8::String::New("btoa"), V8FUNCTION(btoa, this));
    qt->Set(v8::String::New("atob"), V8FUNCTION(atob, this));
    qt->Set(v8::String::New("quit"), V8FUNCTION(quit, this));
    qt->Set(v8::String::New("resolvedUrl"), V8FUNCTION(resolvedUrl, this));

    if (m_engine) {
        qt->Set(v8::String::New("createQmlObject"), V8FUNCTION(createQmlObject, this));
        qt->Set(v8::String::New("createComponent"), V8FUNCTION(createComponent, this));
    }

    // XXX translator functions

    global->Set(v8::String::New("print"), printFn);
    global->Set(v8::String::New("console"), console);
    global->Set(v8::String::New("Qt"), qt);

    // XXX mainthread only
    m_xmlHttpRequestData = qt_add_qmlxmlhttprequest(this);
    m_sqlDatabaseData = qt_add_qmlsqldatabase(this);

    {
    v8::Handle<v8::Value> args[] = { global };
    v8::Local<v8::Value> names = m_getOwnPropertyNames->Call(global, 1, args);
    v8::Local<v8::Array> namesArray = v8::Local<v8::Array>::Cast(names);
    for (quint32 ii = 0; ii < namesArray->Length(); ++ii) 
        m_illegalNames.insert(toString(namesArray->Get(ii)));
    }
}

void QV8Engine::freezeGlobal()
{
    // Freeze the global object 
    // XXX I don't think this is sufficient as it misses non-enumerable properties
#define FREEZE "(function freeze_recur(obj) { "\
               "    if (Qt.isQtObject(obj)) return;"\
               "    for (var prop in obj) { " \
               "        if (prop == \"connect\" || prop == \"disconnect\") {" \
               "            Object.freeze(obj[prop]); "\
               "            continue;" \
               "        }" \
               "        freeze_recur(obj[prop]);" \
               "    }" \
               "    if (obj instanceof Object) {" \
               "        Object.freeze(obj);" \
               "    }"\
               "})(this);"
    v8::Local<v8::Script> test = v8::Script::New(v8::String::New(FREEZE));
#undef FREEZE

    test->Run();
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
    // XXX uses QDeclarativeEngine which means it wont work in worker script?
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
    // XXX worker script?
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
    Q_ASSERT(context);

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

    QObject *obj = component.beginCreate(context->asQDeclarativeContext());
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
    Q_ASSERT(context);

    QString arg = v8engine->toString(args[0]->ToString());
    if (arg.isEmpty())
        return v8::Null();

    QUrl url = context->resolvedUrl(QUrl(arg));
    QDeclarativeComponent *c = new QDeclarativeComponent(engine, url, engine);
    QDeclarativeComponentPrivate::get(c)->creationContext = context;
    QDeclarativeData::get(c, true)->setImplicitDestructible();
    return v8engine->newQObject(c);
}

QT_END_NAMESPACE

