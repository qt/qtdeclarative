/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
** us via http://www.qt-project.org/.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QJSVALUE_H
#define QJSVALUE_H

#include <QtCore/qstring.h>

#include <QtCore/qlist.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QJSValue;
class QJSEngine;
class QVariant;
class QObject;
struct QMetaObject;
class QDateTime;

typedef QList<QJSValue> QJSValueList;

class QJSValuePrivate;
struct QScriptValuePrivatePointerDeleter;
template <class T> class QScriptPassPointer;

class Q_DECLARATIVE_EXPORT QJSValue
{
public:
    enum SpecialValue {
        NullValue,
        UndefinedValue
    };

public:
    QJSValue(SpecialValue value = UndefinedValue);
    ~QJSValue();
    QJSValue(const QJSValue &other);

    QJSValue(bool value);
    QJSValue(int value);
    QJSValue(uint value);
    QJSValue(double value);
    QJSValue(const QString &value);
    QJSValue(const QLatin1String &value);
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN_CONSTRUCTOR QJSValue(const char *str);
#endif

    QJSValue &operator=(const QJSValue &other);

    bool isBool() const;
    bool isNumber() const;
    bool isNull() const;
    bool isString() const;
    bool isUndefined() const;
    bool isVariant() const;
    bool isQObject() const;
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
    QJSValue call(const QJSValueList &args = QJSValueList());
    QJSValue callWithInstance(const QJSValue &instance, const QJSValueList &args = QJSValueList());
    QJSValue callAsConstructor(const QJSValueList &args = QJSValueList());

#ifdef QT_DEPRECATED
    QT_DEPRECATED QJSEngine *engine() const;

    QT_DEPRECATED bool isFunction() const;
#endif

private:
    // force compile error, prevent QJSValue(bool) to be called
    QJSValue(void *);

    QJSValue(QJSValuePrivate*);
    QJSValue(QScriptPassPointer<QJSValuePrivate>);

private:
    QExplicitlySharedDataPointer<QJSValuePrivate> d_ptr;

    Q_DECLARE_PRIVATE(QJSValue)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
