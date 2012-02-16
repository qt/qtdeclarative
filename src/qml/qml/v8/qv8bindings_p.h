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

#ifndef QV8BINDINGS_P_H
#define QV8BINDINGS_P_H

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

#include <private/qqmlpropertycache_p.h>
#include <private/qqmlinstruction_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qqmlcompiler_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qflagpointer_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQmlCompiledData;

class QV8BindingsPrivate;
class QV8Bindings : public QQmlAbstractExpression
{
public:
    QV8Bindings(QQmlCompiledData::V8Program *,
                int line,
                QQmlContextData *context);
    virtual ~QV8Bindings();

    QQmlAbstractBinding *configBinding(QObject *target, QObject *scope,
                                               const QQmlInstruction::instr_assignBinding *);

    // Inherited from QQmlAbstractExpression
    virtual void refresh();

    struct Binding : public QQmlJavaScriptExpression,
                     public QQmlAbstractBinding {
        Binding();

        void update() { QQmlAbstractBinding::update(); }
        void refresh();

        // "Inherited" from QQmlJavaScriptExpression
        static QString expressionIdentifier(QQmlJavaScriptExpression *);
        static void expressionChanged(QQmlJavaScriptExpression *);

        // Inherited from QQmlAbstractBinding
        virtual void setEnabled(bool, QQmlPropertyPrivate::WriteFlags flags);
        virtual void update(QQmlPropertyPrivate::WriteFlags flags);
        virtual void destroy();
        virtual int propertyIndex() const;
        virtual QObject *object() const;

        QObject *target;
        QV8Bindings *parent;

        // To save memory, we store flags inside the instruction pointer.
        //    flag1: enabled
        //    flag2: updating
        QFlagPointer<const QQmlInstruction::instr_assignBinding> instruction;

        inline bool enabledFlag() const { return instruction.flag(); }
        inline void setEnabledFlag(bool v) { instruction.setFlagValue(v); }
        inline bool updatingFlag() const { return instruction.flag2(); }
        inline void setUpdatingFlag(bool v) { instruction.setFlag2Value(v); }
    };

    inline void addref();
    inline void release();

private:
    Q_DISABLE_COPY(QV8Bindings)

    const QUrl &url() const;
    const QString &urlString() const;
    v8::Persistent<v8::Array> &functions() const;

    QQmlCompiledData::V8Program *program;
    Binding *bindings;
    int refCount;
};

void QV8Bindings::addref()
{
    ++refCount;
}

void QV8Bindings::release()
{
    if (0 == --refCount)
        delete this;
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QV8BINDINGS_P_H


