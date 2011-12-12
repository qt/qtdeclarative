/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-ONLY$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCRIPTORIGINALGLOBALOBJECT_P_H
#define QSCRIPTORIGINALGLOBALOBJECT_P_H

#include "QtCore/qglobal.h"
#include "qjsvalue.h"

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QV8Engine;

/*!
    \internal
    This class is a workaround for missing V8 API functionality. This class keeps all important
    properties of an original (default) global object, so we can use it even if the global object was
    changed.

    FIXME this class is a container for workarounds :-) it should be replaced by proper API calls.

    The class have to be created on the QV8Engine creation time (before any change got applied to
    global object).

    \attention All methods (apart from constructor) assumes that a context and a scope are prepared correctly.
*/
class QScriptOriginalGlobalObject
{
public:
    inline QScriptOriginalGlobalObject() {}
    inline void init(v8::Handle<v8::Context> context);
    inline void destroy();

    inline QJSValue::PropertyFlags getPropertyFlags(v8::Handle<v8::Object> object, v8::Handle<v8::Value> property);
    inline v8::Local<v8::Object> getOwnPropertyDescriptor(v8::Handle<v8::Object> object, v8::Handle<v8::Value> property) const;
    inline bool strictlyEquals(v8::Handle<v8::Object> object);
private:
    Q_DISABLE_COPY(QScriptOriginalGlobalObject)

    // Copy of constructors and prototypes used in isType functions.
    v8::Persistent<v8::Function> m_ownPropertyDescriptor;
    v8::Persistent<v8::Object> m_globalObject;
};

void QScriptOriginalGlobalObject::init(v8::Handle<v8::Context> context)
{
    // Please notice that engine is not fully initialized at this point.

    v8::Context::Scope contextScope(context);

    v8::HandleScope scope;

    m_globalObject = v8::Persistent<v8::Object>::New(context->Global());

    v8::Local<v8::Object> objectConstructor = m_globalObject->Get(v8::String::New("Object"))->ToObject();
    Q_ASSERT(objectConstructor->IsObject());
    {   // Initialize m_ownPropertyDescriptor.
        v8::Local<v8::Value> ownPropertyDescriptor = objectConstructor->Get(v8::String::New("getOwnPropertyDescriptor"));
        Q_ASSERT(!ownPropertyDescriptor.IsEmpty());
        m_ownPropertyDescriptor = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(ownPropertyDescriptor));
    }
}

/*!
    \internal
    QScriptOriginalGlobalObject lives as long as QV8Engine that keeps it. In ~QSEP
    the v8 context is removed, so we need to remove our handlers before. to break this dependency
    destroy method should be called before or insight QSEP destructor.
*/
inline void QScriptOriginalGlobalObject::destroy()
{
    m_ownPropertyDescriptor.Dispose();
    m_globalObject.Dispose();
    // After this line this instance is unusable.
}

inline QJSValue::PropertyFlags QScriptOriginalGlobalObject::getPropertyFlags(v8::Handle<v8::Object> object, v8::Handle<v8::Value> property)
{
    Q_ASSERT(object->IsObject());
    Q_ASSERT(!property.IsEmpty());
    v8::Local<v8::Object> descriptor = getOwnPropertyDescriptor(object, property);
    if (descriptor.IsEmpty()) {
//        // Property isn't owned by this object.
//        if (!(mode & QScriptValue::ResolvePrototype))
//            return 0;
        v8::Local<v8::Value> prototype = object->GetPrototype();
        if (prototype->IsNull())
            return 0;
        return getPropertyFlags(v8::Local<v8::Object>::Cast(prototype), property);
    }
    v8::Local<v8::String> writableName = v8::String::New("writable");
    v8::Local<v8::String> configurableName = v8::String::New("configurable");
    v8::Local<v8::String> enumerableName = v8::String::New("enumerable");
//    v8::Local<v8::String> getName = v8::String::New("get");
//    v8::Local<v8::String> setName = v8::String::New("set");

    unsigned flags = 0;

    if (!descriptor->Get(configurableName)->BooleanValue())
        flags |= QJSValue::Undeletable;
    if (!descriptor->Get(enumerableName)->BooleanValue())
        flags |= QJSValue::SkipInEnumeration;

    //"writable" is only a property of the descriptor if it is not an accessor
    if (descriptor->Has(writableName)) {
        if (!descriptor->Get(writableName)->BooleanValue())
            flags |= QJSValue::ReadOnly;
    } else {
//        if (descriptor->Get(getName)->IsObject())
//            flags |= QScriptValue::PropertyGetter;
//        if (descriptor->Get(setName)->IsObject())
//            flags |= QScriptValue::PropertySetter;
    }

    return QJSValue::PropertyFlag(flags);
}

inline v8::Local<v8::Object> QScriptOriginalGlobalObject::getOwnPropertyDescriptor(v8::Handle<v8::Object> object, v8::Handle<v8::Value> property) const
{
    Q_ASSERT(object->IsObject());
    Q_ASSERT(!property.IsEmpty());
    // FIXME do we need try catch here?
    v8::Handle<v8::Value> argv[] = {object, property};
    v8::Local<v8::Value> descriptor = m_ownPropertyDescriptor->Call(m_globalObject, /* argc */ 2, argv);
    if (descriptor.IsEmpty() || !descriptor->IsObject())
        return v8::Local<v8::Object>();
    return v8::Local<v8::Object>::Cast(descriptor);
}

inline bool QScriptOriginalGlobalObject::strictlyEquals(v8::Handle<v8::Object> object)
{
    return m_globalObject->GetPrototype()->StrictEquals(object);
}

QT_END_NAMESPACE

#endif
