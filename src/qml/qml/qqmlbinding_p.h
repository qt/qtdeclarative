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

#ifndef QQMLBINDING_P_H
#define QQMLBINDING_P_H

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

#include "qqml.h"
#include "qqmlpropertyvaluesource.h"
#include "qqmlexpression.h"
#include "qqmlproperty.h"
#include "qqmlproperty_p.h"

#include <QtCore/QObject>
#include <QtCore/QMetaProperty>

#include <private/qpointervaluepair_p.h>
#include <private/qqmlabstractbinding_p.h>
#include <private/qqmlabstractexpression_p.h>
#include <private/qqmljavascriptexpression_p.h>

QT_BEGIN_NAMESPACE

class QQmlContext;
class Q_QML_PRIVATE_EXPORT QQmlBinding : public QQmlJavaScriptExpression,
                                         public QQmlAbstractExpression,
                                         public QQmlAbstractBinding
{
public:
    enum EvaluateFlag { None = 0x00, RequiresThisObject = 0x01 };
    Q_DECLARE_FLAGS(EvaluateFlags, EvaluateFlag)

    QQmlBinding(const QString &, QObject *, QQmlContext *);
    QQmlBinding(const QString &, QObject *, QQmlContextData *);
    QQmlBinding(const QString &, bool isRewritten, QObject *, QQmlContextData *, 
                const QString &url, int lineNumber, int columnNumber);
    QQmlBinding(void *, QObject *, QQmlContextData *,
                const QString &url, int lineNumber, int columnNumber);

    void setTarget(const QQmlProperty &);
    void setTarget(QObject *, const QQmlPropertyData &, QQmlContextData *);
    QQmlProperty property() const;

    void setEvaluateFlags(EvaluateFlags flags);
    EvaluateFlags evaluateFlags() const;

    void setNotifyOnValueChanged(bool);

    // Inherited from  QQmlAbstractExpression
    virtual void refresh();

    // "Inherited" from  QQmlAbstractBinding
    static QString expression(const QQmlAbstractBinding *);
    static int propertyIndex(const QQmlAbstractBinding *);
    static QObject *object(const QQmlAbstractBinding *);
    static void setEnabled(QQmlAbstractBinding *, bool, QQmlPropertyPrivate::WriteFlags);
    static void update(QQmlAbstractBinding *, QQmlPropertyPrivate::WriteFlags);
    static void retargetBinding(QQmlAbstractBinding *, QObject *, int);

    void setEnabled(bool, QQmlPropertyPrivate::WriteFlags flags);
    void update(QQmlPropertyPrivate::WriteFlags flags);
    void update() { update(QQmlPropertyPrivate::DontRemoveBinding); }

    QString expression() const;
    QObject *object() const;
    int propertyIndex() const;
    void retargetBinding(QObject *, int);

    typedef int Identifier;
    static Identifier Invalid;

    static QQmlBinding *createBinding(Identifier, QObject *, QQmlContext *, const QString &, int);

    QVariant evaluate();

    static QString expressionIdentifier(QQmlJavaScriptExpression *);
    static void expressionChanged(QQmlJavaScriptExpression *);

protected:
    friend class QQmlAbstractBinding;
    ~QQmlBinding();

private:
    v8::Persistent<v8::Function> v8function;

    inline bool updatingFlag() const;
    inline void setUpdatingFlag(bool);
    inline bool enabledFlag() const;
    inline void setEnabledFlag(bool);

    struct Retarget {
        QObject *target;
        int targetProperty;
    };

    QPointerValuePair<QObject, Retarget> m_coreObject;
    QQmlPropertyData m_core;
    // We store some flag bits in the following flag pointers.
    //    m_ctxt:flag1 - updatingFlag
    //    m_ctxt:flag2 - enabledFlag
    QFlagPointer<QQmlContextData> m_ctxt;

    // XXX It would be good if we could get rid of these in most circumstances
    QString m_url;
    int m_lineNumber;
    int m_columnNumber;
    QByteArray m_expression;
};

bool QQmlBinding::updatingFlag() const
{
    return m_ctxt.flag();
}

void QQmlBinding::setUpdatingFlag(bool v)
{
    m_ctxt.setFlagValue(v);
}

bool QQmlBinding::enabledFlag() const
{
    return m_ctxt.flag2();
}

void QQmlBinding::setEnabledFlag(bool v)
{
    m_ctxt.setFlag2Value(v);
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlBinding::EvaluateFlags)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlBinding*)

#endif // QQMLBINDING_P_H
