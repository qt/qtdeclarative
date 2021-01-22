/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QJSVALUE_H
#define QJSVALUE_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

class QJSValue;
class QJSEngine;
class QVariant;
class QObject;
struct QMetaObject;
class QDateTime;

typedef QList<QJSValue> QJSValueList;
namespace QV4 {
    struct ExecutionEngine;
    struct Value;
}

class Q_QML_EXPORT QJSValue
{
public:
    enum SpecialValue {
        NullValue,
        UndefinedValue
    };

    enum ErrorType {
        NoError,
        GenericError,
        EvalError,
        RangeError,
        ReferenceError,
        SyntaxError,
        TypeError,
        URIError
    };

public:
    QJSValue(SpecialValue value = UndefinedValue);
    ~QJSValue();
    QJSValue(const QJSValue &other);

#ifdef Q_COMPILER_RVALUE_REFS
    inline QJSValue(QJSValue && other) : d(other.d) { other.d = 0; }
    inline QJSValue &operator=(QJSValue &&other)
    { qSwap(d, other.d); return *this; }
#endif

    QJSValue(bool value);
    QJSValue(int value);
    QJSValue(uint value);
    QJSValue(double value);
    QJSValue(const QString &value);
    QJSValue(const QLatin1String &value);
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN QJSValue(const char *str);
#endif

    QJSValue &operator=(const QJSValue &other);

    bool isBool() const;
    bool isNumber() const;
    bool isNull() const;
    bool isString() const;
    bool isUndefined() const;
    bool isVariant() const;
    bool isQObject() const;
    bool isQMetaObject() const;
    bool isObject() const;
    bool isDate() const;
    bool isRegExp() const;
    bool isArray() const;
    bool isError() const;

    QString toString() const;
    double toNumber() const;
    qint32 toInt() const;
    quint32 toUInt() const;
    bool toBool() const;
    QVariant toVariant() const;
    QObject *toQObject() const;
    const QMetaObject *toQMetaObject() const;
    QDateTime toDateTime() const;

    bool equals(const QJSValue &other) const;
    bool strictlyEquals(const QJSValue &other) const;

    QJSValue prototype() const;
    void setPrototype(const QJSValue &prototype);

    QJSValue property(const QString &name) const;
    void setProperty(const QString &name, const QJSValue &value);

    bool hasProperty(const QString &name) const;
    bool hasOwnProperty(const QString &name) const;

    QJSValue property(quint32 arrayIndex) const;
    void setProperty(quint32 arrayIndex, const QJSValue &value);

    bool deleteProperty(const QString &name);

    bool isCallable() const;
    QJSValue call(const QJSValueList &args = QJSValueList()); // ### Qt6: Make const
    QJSValue callWithInstance(const QJSValue &instance, const QJSValueList &args = QJSValueList()); // ### Qt6: Make const
    QJSValue callAsConstructor(const QJSValueList &args = QJSValueList()); // ### Qt6: Make const

    ErrorType errorType() const;
#ifdef QT_DEPRECATED
    QT_DEPRECATED QJSEngine *engine() const;
#endif

    QJSValue(QV4::ExecutionEngine *e, quint64 val);
private:
    friend class QJSValuePrivate;
    // force compile error, prevent QJSValue(bool) to be called
    QJSValue(void *) Q_DECL_EQ_DELETE;

    mutable quintptr d;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QJSValue)

#endif
