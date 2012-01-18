/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
** us via http://www.qt-project.org/.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QJSVALUEITERATOR_IMPL_P_H
#define QJSVALUEITERATOR_IMPL_P_H

#include "qjsvalueiterator_p.h"
#include <private/qv8engine_p.h>
#include "qjsconverter_p.h"

inline QJSValueIteratorPrivate::QJSValueIteratorPrivate(const QJSValuePrivate* value)
    : m_object(const_cast<QJSValuePrivate*>(value))
    , m_index(0)
    , m_count(0)
{
    Q_ASSERT(value);
    QV8Engine *engine = m_object->engine();
    if (!m_object->isObject())
        m_object = 0;
    else {
        QScriptIsolate api(engine, QScriptIsolate::NotNullEngine);
        v8::HandleScope scope;

        v8::Handle<v8::Value> tmp = *value;
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(tmp);
        v8::Local<v8::Array> names;

        // FIXME we need newer V8!
        //names = obj->GetOwnPropertyNames();
        names = engine->getOwnPropertyNames(obj);
        m_names = v8::Persistent<v8::Array>::New(names);
        m_count = names->Length();

        engine->registerValueIterator(this);
    }
}

inline QJSValueIteratorPrivate::~QJSValueIteratorPrivate()
{
    if (isValid()) {
        engine()->unregisterValueIterator(this);
        m_names.Dispose();
    }
}

inline void QJSValueIteratorPrivate::invalidate()
{
    m_names.Dispose();
    m_object.reset();
    m_index = 0;
    m_count = 0;
}

inline bool QJSValueIteratorPrivate::hasNext() const
{
    return isValid() ? m_index < m_count : false;
}

inline bool QJSValueIteratorPrivate::next()
{
    if (hasNext()) {
        ++m_index;
        return true;
    }
    return false;
}

inline QString QJSValueIteratorPrivate::name() const
{
    if (!isValid())
        return QString();

    v8::HandleScope handleScope;
    return QJSConverter::toString(m_names->Get(m_index - 1)->ToString());
}

inline QScriptPassPointer<QJSValuePrivate> QJSValueIteratorPrivate::value() const
{
    if (!isValid())
        return new QJSValuePrivate();

    v8::HandleScope handleScope;
    return m_object->property(m_names->Get(m_index - 1)->ToString());
}

inline bool QJSValueIteratorPrivate::isValid() const
{
    bool result = m_object ? !m_object->isUndefined() : false;
    // We know that if this object is still valid then it is an object
    // if this assumption is not correct then some other logic in this class
    // have to be changed too.
    Q_ASSERT(!result || m_object->isObject());
    return result;
}

inline QV8Engine* QJSValueIteratorPrivate::engine() const
{
    return m_object ? m_object->engine() : 0;
}

#endif // QJSVALUEITERATOR_IMPL_P_H
