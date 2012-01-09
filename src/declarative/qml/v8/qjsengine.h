/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QJSENGINE_H
#define QJSENGINE_H

#include <QtCore/qmetatype.h>

#include <QtCore/qvariant.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qobject.h>
#include <QtDeclarative/qjsvalue.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDateTime;
class QV8Engine;

class QRegExp;

template <typename T>
inline T qjsvalue_cast(const QJSValue &);

class QJSEnginePrivate;
class Q_DECLARATIVE_EXPORT QJSEngine
    : public QObject
{
    Q_OBJECT
public:
    enum ContextOwnership {
        AdoptCurrentContext,
        CreateNewContext
    };

    QJSEngine();
    explicit QJSEngine(ContextOwnership ownership);
    explicit QJSEngine(QObject *parent);
    virtual ~QJSEngine();

    QJSValue globalObject() const;

    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1);

    bool hasUncaughtException() const;
    QJSValue uncaughtException() const;
    void clearExceptions();

    QJSValue nullValue();
    QJSValue undefinedValue();

    QJSValue newVariant(const QVariant &value);

    QJSValue newRegExp(const QRegExp &regexp);

    QJSValue newObject();
    QJSValue newArray(uint length = 0);
    QJSValue newRegExp(const QString &pattern, const QString &flags);
    QJSValue newDate(double value);
    QJSValue newDate(const QDateTime &value);

    QJSValue newQObject(QObject *object);

    template <typename T>
    inline QJSValue toScriptValue(const T &value)
    {
        return create(qMetaTypeId<T>(), &value);
    }
    template <typename T>
    inline T fromScriptValue(const QJSValue &value)
    {
        return qjsvalue_cast<T>(value);
    }

    void collectGarbage();

    QJSValue toObject(const QJSValue &value);

    QV8Engine *handle() const { return d; }

Q_SIGNALS:
    void signalHandlerException(const QJSValue &exception);

private:
    QJSValue create(int type, const void *ptr);

    static bool convertV2(const QJSValue &value, int type, void *ptr);

    friend inline bool qjsvalue_cast_helper(const QJSValue &, int, void *);

protected:
    QJSEngine(QJSEnginePrivate &dd, QObject *parent = 0);

private:
    QV8Engine *d;
    Q_DISABLE_COPY(QJSEngine)
    Q_DECLARE_PRIVATE(QJSEngine)
    friend class QV8Engine;
};

inline bool qjsvalue_cast_helper(const QJSValue &value, int type, void *ptr)
{
    return QJSEngine::convertV2(value, type, ptr);
}

template<typename T>
T qjsvalue_cast(const QJSValue &value)
{
    T t;
    const int id = qMetaTypeId<T>();

    if (qjsvalue_cast_helper(value, id, &t))
        return t;
    else if (value.isVariant())
        return qvariant_cast<T>(value.toVariant());

    return T();
}

template <>
inline QVariant qjsvalue_cast<QVariant>(const QJSValue &value)
{
    return value.toVariant();
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QJSENGINE_H
