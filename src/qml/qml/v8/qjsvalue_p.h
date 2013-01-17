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

#ifndef QJSVALUE_P_H
#define QJSVALUE_P_H

#include <private/qv8_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmath.h>
#include <QtCore/qvarlengtharray.h>
#include <qdebug.h>

#include <private/qintrusivelist_p.h>
#include "qscriptshareddata_p.h"
#include "qjsvalue.h"

QT_BEGIN_NAMESPACE

class QV8Engine;

/*!
  \internal
  \class QJSValuePrivate
*/
class QJSValuePrivate
        : public QSharedData
{
public:
    enum PropertyFlag {
        ReadOnly            = 0x00000001,
        Undeletable         = 0x00000002,
        SkipInEnumeration   = 0x00000004
    };
    Q_DECLARE_FLAGS(PropertyFlags, PropertyFlag)

    inline static QJSValuePrivate* get(const QJSValue& q);
    inline static QJSValue get(const QJSValuePrivate* d);
    inline static QJSValue get(QJSValuePrivate* d);
    inline static QJSValue get(QScriptPassPointer<QJSValuePrivate> d);
    inline ~QJSValuePrivate();

    inline QJSValuePrivate(bool value);
    inline QJSValuePrivate(int value);
    inline QJSValuePrivate(uint value);
    inline QJSValuePrivate(double value);
    inline QJSValuePrivate(const QString& value);
    inline QJSValuePrivate(QJSValue::SpecialValue value = QJSValue::UndefinedValue);

    inline QJSValuePrivate(QV8Engine *engine, bool value);
    inline QJSValuePrivate(QV8Engine *engine, int value);
    inline QJSValuePrivate(QV8Engine *engine, uint value);
    inline QJSValuePrivate(QV8Engine *engine, double value);
    inline QJSValuePrivate(QV8Engine *engine, const QString& value);
    inline QJSValuePrivate(QV8Engine *engine, QJSValue::SpecialValue value = QJSValue::UndefinedValue);
    inline QJSValuePrivate(QV8Engine *engine, v8::Handle<v8::Value>);
    inline void invalidate();

    inline bool toBool() const;
    inline double toNumber() const;
    inline QString toString() const;
    inline double toInteger() const;
    inline qint32 toInt32() const;
    inline quint32 toUInt32() const;
    inline quint16 toUInt16() const;
    inline QDateTime toDataTime() const;
    inline QObject *toQObject() const;
    inline QVariant toVariant() const;

    inline bool isArray() const;
    inline bool isBool() const;
    inline bool isCallable() const;
    inline bool isError() const;
    inline bool isFunction() const;
    inline bool isNull() const;
    inline bool isNumber() const;
    inline bool isObject() const;
    inline bool isString() const;
    inline bool isUndefined() const;
    inline bool isVariant() const;
    inline bool isDate() const;
    inline bool isRegExp() const;
    inline bool isQObject() const;

    inline bool equals(QJSValuePrivate* other);
    inline bool strictlyEquals(QJSValuePrivate* other);

    inline QScriptPassPointer<QJSValuePrivate> prototype() const;
    inline void setPrototype(QJSValuePrivate* prototype);

    inline void setProperty(const QString &name, QJSValuePrivate *value, uint attribs = 0);
    inline void setProperty(v8::Handle<v8::String> name, QJSValuePrivate *value, uint attribs = 0);
    inline void setProperty(quint32 index, QJSValuePrivate* value, uint attribs = 0);
    inline QScriptPassPointer<QJSValuePrivate> property(const QString& name) const;
    inline QScriptPassPointer<QJSValuePrivate> property(v8::Handle<v8::String> name) const;
    inline QScriptPassPointer<QJSValuePrivate> property(quint32 index) const;
    template<typename T>
    inline QScriptPassPointer<QJSValuePrivate> property(T name) const;
    inline bool deleteProperty(const QString& name);
    inline bool hasProperty(const QString &name) const;
    inline bool hasOwnProperty(const QString &name) const;
    inline PropertyFlags propertyFlags(const QString& name) const;
    inline PropertyFlags propertyFlags(v8::Handle<v8::String> name) const;

    inline QScriptPassPointer<QJSValuePrivate> call(QJSValuePrivate* thisObject, const QJSValueList& args);
    inline QScriptPassPointer<QJSValuePrivate> call(QJSValuePrivate* thisObject, const QJSValue& arguments);
    inline QScriptPassPointer<QJSValuePrivate> call(QJSValuePrivate* thisObject, int argc, v8::Handle< v8::Value >* argv);
    inline QScriptPassPointer<QJSValuePrivate> callAsConstructor(int argc, v8::Handle<v8::Value> *argv);
    inline QScriptPassPointer<QJSValuePrivate> callAsConstructor(const QJSValueList& args);
    inline QScriptPassPointer<QJSValuePrivate> callAsConstructor(const QJSValue& arguments);

    inline bool assignEngine(QV8Engine *engine);
    inline QV8Engine *engine() const;

    inline operator v8::Handle<v8::Value>() const;
    inline operator v8::Handle<v8::Object>() const;
    inline v8::Handle<v8::Value> handle() const;
    inline v8::Handle<v8::Value> asV8Value(QV8Engine *engine);
private:
    QIntrusiveListNode m_node;
    QV8Engine *m_engine;

    // Please, update class documentation when you change the enum.
    enum State {
        CString = 0x1000,
        CNumber,
        CBool,
        CNull,
        CUndefined,
        JSValue = 0x2000 // V8 values are equal or higher then this value.
        // JSPrimitive,
        // JSObject
    } m_state;

    union CValue {
        bool m_bool;
        double m_number;
        QString* m_string;

        CValue() : m_number(0) {}
        CValue(bool value) : m_bool(value) {}
        CValue(int number) : m_number(number) {}
        CValue(uint number) : m_number(number) {}
        CValue(double number) : m_number(number) {}
        CValue(QString* string) : m_string(string) {}
    } u;
    // v8::Persistent is not a POD, so can't be part of the union.
    v8::Persistent<v8::Value> m_value;

    Q_DISABLE_COPY(QJSValuePrivate)
    inline bool isJSBased() const;
    inline bool isNumberBased() const;
    inline bool isStringBased() const;
    inline bool prepareArgumentsForCall(v8::Handle<v8::Value> argv[], const QJSValueList& arguments) const;

    friend class QV8Engine;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QJSValuePrivate::PropertyFlags)

QT_END_NAMESPACE

#endif
