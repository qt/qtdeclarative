/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLEXPRESSION_H
#define QQMLEXPRESSION_H

#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlscriptstring.h>

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>

QT_BEGIN_HEADER

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
    QQmlExpression(QQmlContext *, QObject *, const QString &, QObject * = 0);
    explicit QQmlExpression(const QQmlScriptString &, QQmlContext * = 0, QObject * = 0, QObject * = 0);
    virtual ~QQmlExpression();

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

    QVariant evaluate(bool *valueIsUndefined = 0);

Q_SIGNALS:
    void valueChanged();

protected:
    QQmlExpression(QQmlContextData *, QObject *, const QString &,
                           QQmlExpressionPrivate &dd);
    QQmlExpression(QQmlContextData *, QObject *, const QString &, bool,
                           const QString &, int, int, QQmlExpressionPrivate &dd);
    QQmlExpression(QQmlContextData *, QObject *, const QByteArray &, bool,
                           const QString &, int, int, QQmlExpressionPrivate &dd);

private:
    QQmlExpression(QQmlContextData *, QObject *, const QString &);

    Q_DISABLE_COPY(QQmlExpression)
    Q_DECLARE_PRIVATE(QQmlExpression)
    friend class QQmlDebugger;
    friend class QQmlContext;
    friend class QQmlVME;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQMLEXPRESSION_H

