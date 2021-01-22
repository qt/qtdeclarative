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

#ifndef QQMLEXPRESSION_H
#define QQMLEXPRESSION_H

#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlscriptstring.h>

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE


class QString;
class QQmlRefCount;
class QQmlEngine;
class QQmlContext;
class QQmlExpressionPrivate;
class QQmlContextData;
class Q_QML_EXPORT QQmlExpression : public QObject
{
    Q_OBJECT
public:
    QQmlExpression();
    QQmlExpression(QQmlContext *, QObject *, const QString &, QObject * = nullptr);
    explicit QQmlExpression(const QQmlScriptString &, QQmlContext * = nullptr, QObject * = nullptr, QObject * = nullptr);
    ~QQmlExpression() override;

    QQmlEngine *engine() const;
    QQmlContext *context() const;

    QString expression() const;
    void setExpression(const QString &);

    bool notifyOnValueChanged() const;
    void setNotifyOnValueChanged(bool);

    QString sourceFile() const;
    int lineNumber() const;
    int columnNumber() const;
    void setSourceLocation(const QString &fileName, int line, int column = 0);

    QObject *scopeObject() const;

    bool hasError() const;
    void clearError();
    QQmlError error() const;

    QVariant evaluate(bool *valueIsUndefined = nullptr);

Q_SIGNALS:
    void valueChanged();

private:
    QQmlExpression(QQmlContextData *, QObject *, const QString &);

    Q_DISABLE_COPY(QQmlExpression)
    Q_DECLARE_PRIVATE(QQmlExpression)
    friend class QQmlDebugger;
    friend class QQmlContext;
};

QT_END_NAMESPACE

#endif // QQMLEXPRESSION_H

