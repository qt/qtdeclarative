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

#ifndef QDECLARATIVEV8ENGINE_P_H
#define QDECLARATIVEV8ENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/qset.h>
#include <private/qv8_p.h>

#include <private/qdeclarativepropertycache_p.h>

#include "qv8contextwrapper_p.h"
#include "qv8qobjectwrapper_p.h"
#include "qv8stringwrapper_p.h"
#include "qv8typewrapper_p.h"
#include "qv8listwrapper_p.h"
#include "qv8variantwrapper_p.h"
#include "qv8valuetypewrapper_p.h"

QT_BEGIN_NAMESPACE

#define V8_RESOURCE_TYPE(resourcetype) \
public: \
    enum { V8ResourceType = QV8ObjectResource:: resourcetype }; \
    virtual QV8ObjectResource::ResourceType resourceType() const { return QV8ObjectResource:: resourcetype; } \
private: 

#define V8ENGINE() ((QV8Engine *)v8::External::Unwrap(args.Data()))
#define V8FUNCTION(function, engine) v8::FunctionTemplate::New(function, v8::External::Wrap((QV8Engine*)engine))->GetFunction()
// XXX Are we mean to return a value here, or is an empty handle ok?
#define V8THROW_ERROR(string) { \
    v8::ThrowException(v8::Exception::Error(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}

class QV8Engine;
class QV8ObjectResource : public v8::Object::ExternalResource
{
public:
    QV8ObjectResource(QV8Engine *engine) : engine(engine) { Q_ASSERT(engine); }
    enum ResourceType { ContextType, QObjectType, TypeType, ListType, VariantType, 
                        ValueTypeType, XMLHttpRequestType, DOMNodeType, SQLDatabaseType };
    virtual ResourceType resourceType() const = 0;

    QV8Engine *engine;
};

template<class T>
T *v8_resource_cast(v8::Handle<v8::Object> object) {
    QV8ObjectResource *resource = static_cast<QV8ObjectResource *>(object->GetExternalResource());
    return (resource && (quint32)resource->resourceType() == (quint32)T::V8ResourceType)?static_cast<T *>(resource):0;
}

// Used to allow a QObject method take and return raw V8 handles without having to expose
// v8 in the public API.
// Use like this:
//     class MyClass : public QObject {
//         Q_OBJECT
//         ...
//         Q_INVOKABLE void myMethod(QDeclarativeV8Function*);
//     };
// The QDeclarativeV8Function - and consequently the arguments and return value - only remains 
// valid during the call.  If the return value isn't set within myMethod(), the will return
// undefined.
class QV8Engine;
class QDeclarativeV8Function
{
public:
    int Length() const { return _ac; }
    v8::Local<v8::Value> operator[](int idx) { return (*_a)->Get(idx); }
    QDeclarativeContextData *context() { return _c; }
    v8::Handle<v8::Object> qmlGlobal() { return *_g; }
    void returnValue(v8::Handle<v8::Value> rv) { *_r = rv; }
    QV8Engine *engine() const { return _e; }
private:
    friend class QV8QObjectWrapper;
    QDeclarativeV8Function();
    QDeclarativeV8Function(const QDeclarativeV8Function &);
    QDeclarativeV8Function &operator=(const QDeclarativeV8Function &);

    QDeclarativeV8Function(int length, v8::Handle<v8::Object> &args, 
                           v8::Handle<v8::Value> &rv, v8::Handle<v8::Object> &global,
                           QDeclarativeContextData *c, QV8Engine *e)
    : _ac(length), _a(&args), _r(&rv), _g(&global), _c(c), _e(e) {}

    int _ac;
    v8::Handle<v8::Object> *_a;
    v8::Handle<v8::Value> *_r;
    v8::Handle<v8::Object> *_g;
    QDeclarativeContextData *_c;
    QV8Engine *_e;
};

class QDeclarativeV8Handle
{
public:
    QDeclarativeV8Handle() : d(0) {}
    QDeclarativeV8Handle(const QDeclarativeV8Handle &other) : d(other.d) {}
    QDeclarativeV8Handle &operator=(const QDeclarativeV8Handle &other) { d = other.d; return *this; }

    static QDeclarativeV8Handle fromHandle(v8::Handle<v8::Value> h) {
        return reinterpret_cast<QDeclarativeV8Handle &>(h);
    }
    v8::Handle<v8::Value> toHandle() const {
        return reinterpret_cast<const v8::Handle<v8::Value> &>(*this);
    }
private:
    void *d;
};

class QObject;
class QDeclarativeEngine;
class QDeclarativeValueType;
class QNetworkAccessManager;
class QDeclarativeContextData;
class Q_AUTOTEST_EXPORT QV8Engine 
{
public:
    QV8Engine();
    ~QV8Engine();

    void init(QDeclarativeEngine *);

    QDeclarativeEngine *engine() { return m_engine; }
    v8::Local<v8::Object> global() { return m_context->Global(); }
    v8::Handle<v8::Context> context() { return m_context; }
    QV8ContextWrapper *contextWrapper() { return &m_contextWrapper; }
    QV8QObjectWrapper *qobjectWrapper() { return &m_qobjectWrapper; }
    QV8TypeWrapper *typeWrapper() { return &m_typeWrapper; }
    QV8ListWrapper *listWrapper() { return &m_listWrapper; }
    QV8VariantWrapper *variantWrapper() { return &m_variantWrapper; }

    void *xmlHttpRequestData() { return m_xmlHttpRequestData; }
    void *sqlDatabaseData() { return m_sqlDatabaseData; }

    QDeclarativeContextData *callingContext();

    v8::Local<v8::Array> getOwnPropertyNames(v8::Handle<v8::Object>);

    inline QString toString(v8::Handle<v8::Value> string);
    inline QString toString(v8::Handle<v8::String> string);
    static QString toStringStatic(v8::Handle<v8::Value>);
    static QString toStringStatic(v8::Handle<v8::String>);
    static inline bool startsWithUpper(v8::Handle<v8::String>);

    QVariant toVariant(v8::Handle<v8::Value>, int typeHint);
    v8::Handle<v8::Value> fromVariant(const QVariant &);
    inline bool isVariant(v8::Handle<v8::Value>);

    // Compile \a source (from \a fileName at \a lineNumber) in QML mode
    v8::Local<v8::Script> qmlModeCompile(const QString &source, 
                                         const QString &fileName = QString(), 
                                         int lineNumber = 1);

    // Return the QML global "scope" object for the \a ctxt context and \a scope object.
    inline v8::Local<v8::Object> qmlScope(QDeclarativeContextData *ctxt, QObject *scope);

    // Return a JS wrapper for the given QObject \a object
    inline v8::Handle<v8::Value> newQObject(QObject *object);
    inline bool isQObject(v8::Handle<v8::Value>);
    inline QObject *toQObject(v8::Handle<v8::Value>);

    // Return a JS string for the given QString \a string
    inline v8::Local<v8::String> toString(const QString &string);

    // Create a new value type object
    inline v8::Handle<v8::Value> newValueType(QObject *, int coreIndex, QDeclarativeValueType *);
    inline v8::Handle<v8::Value> newValueType(const QVariant &, QDeclarativeValueType *);

    // Create a new QVariant object.  This doesn't examine the type of the variant, but always returns
    // a QVariant wrapper
    inline v8::Handle<v8::Value> newQVariant(const QVariant &);

    // Return the network access manager for this engine.  By default this returns the network
    // access manager of the QDeclarativeEngine.  It is overridden in the case of a threaded v8
    // instance (like in WorkerScript).
    virtual QNetworkAccessManager *networkAccessManager();

    // Return the list of illegal id names (the names of the properties on the global object)
    const QSet<QString> &illegalNames() const;

private:
    QDeclarativeEngine *m_engine;
    v8::Persistent<v8::Context> m_context;

    QV8StringWrapper m_stringWrapper;
    QV8ContextWrapper m_contextWrapper;
    QV8QObjectWrapper m_qobjectWrapper;
    QV8TypeWrapper m_typeWrapper;
    QV8ListWrapper m_listWrapper;
    QV8VariantWrapper m_variantWrapper;
    QV8ValueTypeWrapper m_valueTypeWrapper;

    v8::Persistent<v8::Function> m_getOwnPropertyNames;

    void *m_xmlHttpRequestData;
    void *m_sqlDatabaseData;

    QSet<QString> m_illegalNames;

    QVariant toBasicVariant(v8::Handle<v8::Value>);

    void initializeGlobal(v8::Handle<v8::Object>);
    void freezeGlobal();

    static v8::Handle<v8::Value> print(const v8::Arguments &args);
    static v8::Handle<v8::Value> isQtObject(const v8::Arguments &args);
    static v8::Handle<v8::Value> rgba(const v8::Arguments &args);
    static v8::Handle<v8::Value> hsla(const v8::Arguments &args);
    static v8::Handle<v8::Value> rect(const v8::Arguments &args);
    static v8::Handle<v8::Value> point(const v8::Arguments &args);
    static v8::Handle<v8::Value> size(const v8::Arguments &args);
    static v8::Handle<v8::Value> vector3d(const v8::Arguments &args);
    static v8::Handle<v8::Value> lighter(const v8::Arguments &args);
    static v8::Handle<v8::Value> darker(const v8::Arguments &args);
    static v8::Handle<v8::Value> tint(const v8::Arguments &args);
    static v8::Handle<v8::Value> formatDate(const v8::Arguments &args);
    static v8::Handle<v8::Value> formatTime(const v8::Arguments &args);
    static v8::Handle<v8::Value> formatDateTime(const v8::Arguments &args);
    static v8::Handle<v8::Value> openUrlExternally(const v8::Arguments &args);
    static v8::Handle<v8::Value> fontFamilies(const v8::Arguments &args);
    static v8::Handle<v8::Value> md5(const v8::Arguments &args);
    static v8::Handle<v8::Value> btoa(const v8::Arguments &args);
    static v8::Handle<v8::Value> atob(const v8::Arguments &args);
    static v8::Handle<v8::Value> quit(const v8::Arguments &args);
    static v8::Handle<v8::Value> resolvedUrl(const v8::Arguments &args);
    static v8::Handle<v8::Value> createQmlObject(const v8::Arguments &args);
    static v8::Handle<v8::Value> createComponent(const v8::Arguments &args);

    double qtDateTimeToJsDate(const QDateTime &dt);
    QDateTime qtDateTimeFromJsDate(double jsDate);
};

QString QV8Engine::toString(v8::Handle<v8::Value> string)
{
    return m_stringWrapper.toString(string->ToString());
}

QString QV8Engine::toString(v8::Handle<v8::String> string)
{
    return m_stringWrapper.toString(string);
}

bool QV8Engine::isVariant(v8::Handle<v8::Value> value)
{
    return m_variantWrapper.isVariant(value);
}

v8::Local<v8::Object> QV8Engine::qmlScope(QDeclarativeContextData *ctxt, QObject *scope)
{
    return m_contextWrapper.qmlScope(ctxt, scope);
}

bool QV8Engine::isQObject(v8::Handle<v8::Value> obj)
{
    return obj->IsObject()?m_qobjectWrapper.isQObject(v8::Handle<v8::Object>::Cast(obj)):false;
}

QObject *QV8Engine::toQObject(v8::Handle<v8::Value> obj)
{
    return obj->IsObject()?m_qobjectWrapper.toQObject(v8::Handle<v8::Object>::Cast(obj)):0;
}

v8::Handle<v8::Value> QV8Engine::newQObject(QObject *object)
{
    return m_qobjectWrapper.newQObject(object);
}

v8::Local<v8::String> QV8Engine::toString(const QString &string)
{
    return m_stringWrapper.toString(string);
}

v8::Handle<v8::Value> QV8Engine::newValueType(QObject *object, int property, QDeclarativeValueType *type)
{
    return m_valueTypeWrapper.newValueType(object, property, type);
}

v8::Handle<v8::Value> QV8Engine::newValueType(const QVariant &value, QDeclarativeValueType *type)
{
    return m_valueTypeWrapper.newValueType(value, type);
}

// XXX perf?
bool QV8Engine::startsWithUpper(v8::Handle<v8::String> string)
{
    uint16_t buffer[2];
    int written = string->Write(buffer, 0, 1);
    if (written == 0) return false;
    uint16_t c = buffer[0];
    return ((c != '_' ) && (!(c >= 'a' && c <= 'z')) &&
           ((c >= 'A' && c <= 'Z') || QChar::category(c) == QChar::Letter_Uppercase));
}

QT_END_NAMESPACE

#endif // QDECLARATIVEV8ENGINE_P_H
