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

#ifndef QV8SEQUENCEWRAPPER_P_H
#define QV8SEQUENCEWRAPPER_P_H

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
#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QV8Engine;
class QV8ObjectResource;

class QV8SequenceWrapper
{
public:
    QV8SequenceWrapper();
    ~QV8SequenceWrapper();

    void init(QV8Engine *);
    void destroy();

    bool isSequenceType(int sequenceTypeId) const;

    bool isEqual(QV8ObjectResource *lhs, const QVariant &rhs);
    bool isEqual(QV8ObjectResource *lhs, QV8ObjectResource *rhs);
    quint32 sequenceLength(QV8ObjectResource *);

    v8::Local<v8::Object> newSequence(int sequenceTypeId, QObject *object, int propertyIndex, bool *succeeded);
    v8::Local<v8::Object> fromVariant(const QVariant& v, bool *succeeded);
    QVariant toVariant(QV8ObjectResource *);
    QVariant toVariant(v8::Handle<v8::Array> array, int typeHint, bool *succeeded);

private:
    QV8Engine *m_engine;

    v8::Persistent<v8::Function> m_constructor;
    v8::Persistent<v8::Function> m_toString;
    v8::Persistent<v8::Function> m_valueOf;
    v8::Persistent<v8::Function> m_sort;
    v8::Persistent<v8::Value> m_arrayPrototype;
    v8::Persistent<v8::Function> m_defaultSortComparer;

    static v8::Handle<v8::Value> IndexedGetter(quint32 index, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> IndexedSetter(quint32 index, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Boolean> IndexedDeleter(quint32 index, const v8::AccessorInfo &info);
    static v8::Handle<v8::Array> IndexedEnumerator(const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> LengthGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    static void LengthSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> ToStringGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> ToString(const v8::Arguments &args);
    static v8::Handle<v8::Value> ValueOfGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> SortGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> ValueOf(const v8::Arguments &args);
    static v8::Handle<v8::Value> Getter(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Setter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Sort(const v8::Arguments &args);
};


QT_END_NAMESPACE

#endif // QV8SEQUENCEWRAPPER_P_H
