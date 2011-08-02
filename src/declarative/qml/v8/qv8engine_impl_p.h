/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#ifndef QV8ENGINE_IMPL_P_H
#define QV8ENGINE_IMPL_P_H

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

#include "qv8engine_p.h"
#include "qjsvalue_p.h"
#include "qjsconverter_p.h"
#include "qjsvalueiterator_p.h"

QT_BEGIN_NAMESPACE

inline v8::Handle<v8::Value> QV8Engine::makeJSValue(bool value)
{
    return value ? v8::True() : v8::False();
}

inline v8::Handle<v8::Value> QV8Engine::makeJSValue(int value)
{
    return v8::Integer::New(value);
}

inline v8::Handle<v8::Value> QV8Engine::makeJSValue(uint value)
{
    return v8::Integer::NewFromUnsigned(value);
}

inline v8::Handle<v8::Value> QV8Engine::makeJSValue(double value)
{
    return v8::Number::New(value);
}

inline v8::Handle<v8::Value> QV8Engine::makeJSValue(QJSValue::SpecialValue value) {
    if (value == QJSValue::NullValue)
        return v8::Null();
    return v8::Undefined();
}

inline v8::Handle<v8::Value> QV8Engine::makeJSValue(const QString& value)
{
    return QJSConverter::toString(value);
}

class QtScriptBagCleaner
{
public:
    template<class T>
    void operator () (T* value) const
    {
        value->reinitialize();
    }
    void operator () (QJSValueIteratorPrivate *iterator) const
    {
        iterator->invalidate();
    }
};

inline void QV8Engine::registerValue(QJSValuePrivate *data)
{
    m_values.insert(data);
}

inline void QV8Engine::unregisterValue(QJSValuePrivate *data)
{
    m_values.remove(data);
}

inline void QV8Engine::invalidateAllValues()
{
    QtScriptBagCleaner invalidator;
    m_values.forEach(invalidator);
    m_values.clear();
}

inline void QV8Engine::registerValueIterator(QJSValueIteratorPrivate *data)
{
    m_valueIterators.insert(data);
}

inline void QV8Engine::unregisterValueIterator(QJSValueIteratorPrivate *data)
{
    m_valueIterators.remove(data);
}

inline void QV8Engine::invalidateAllIterators()
{
    QtScriptBagCleaner invalidator;
    m_valueIterators.forEach(invalidator);
    m_valueIterators.clear();
}

/*!
  \internal
  \note property can be index (v8::Integer) or a property (v8::String) name, according to ECMA script
  property would be converted to a string.
*/
inline QJSValue::PropertyFlags QV8Engine::getPropertyFlags(v8::Handle<v8::Object> object, v8::Handle<v8::Value> property)
{
    QJSValue::PropertyFlags flags = m_originalGlobalObject.getPropertyFlags(object, property);
    return flags;
}

QScriptPassPointer<QJSValuePrivate> QV8Engine::evaluate(const QString& program, const QString& fileName, int lineNumber)
{
    v8::TryCatch tryCatch;
    v8::ScriptOrigin scriptOrigin(QJSConverter::toString(fileName), v8::Integer::New(lineNumber - 1));
    v8::Handle<v8::Script> script;
    script = v8::Script::Compile(QJSConverter::toString(program), &scriptOrigin);
    if (script.IsEmpty()) {
        // TODO: Why don't we get the exception, as with Script::Compile()?
        // Q_ASSERT(tryCatch.HasCaught());
        v8::Handle<v8::Value> error = v8::Exception::SyntaxError(v8::String::New(""));
        setException(error);
        return new QJSValuePrivate(this, error);
    }
    return evaluate(script, tryCatch);
}

QT_END_NAMESPACE

#endif // QV8ENGINE_IMPL_P_H
