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

#ifndef QQMLSCRIPTSTRING_H
#define QQMLSCRIPTSTRING_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QObject;
class QQmlContext;
class QQmlScriptStringPrivate;
class QQmlObjectCreator;
namespace QV4 {
    struct QObjectWrapper;
}
class Q_QML_EXPORT QQmlScriptString
{
public:
    QQmlScriptString();
    QQmlScriptString(const QQmlScriptString &);
    ~QQmlScriptString();

    QQmlScriptString &operator=(const QQmlScriptString &);

    bool operator==(const QQmlScriptString &) const;
    bool operator!=(const QQmlScriptString &) const;

    bool isEmpty() const;

    bool isUndefinedLiteral() const;
    bool isNullLiteral() const;
    QString stringLiteral() const;
    qreal numberLiteral(bool *ok) const;
    bool booleanLiteral(bool *ok) const;

private:
    QQmlScriptString(const QString &script, QQmlContext *context, QObject *scope);
    QSharedDataPointer<QQmlScriptStringPrivate> d;

    friend class QQmlObjectCreator;
    friend class QQmlScriptStringPrivate;
    friend class QQmlExpression;
    friend class QQmlBinding;
    friend struct QV4::QObjectWrapper;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlScriptString)

#endif // QQMLSCRIPTSTRING_H

