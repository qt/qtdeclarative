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

#ifndef QJSVALUEITERATOR_IMPL_P_H
#define QJSVALUEITERATOR_IMPL_P_H

#include "qjsvalueiterator_p.h"
#include <private/qv8engine_p.h>
#include "qjsconverter_p.h"

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE

#endif // QJSVALUEITERATOR_IMPL_P_H
