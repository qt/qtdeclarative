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

#ifndef QV4BINDINGS_P_H
#define QV4BINDINGS_P_H

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

#include "private/qqmlexpression_p.h"
#include "private/qqmlbinding_p.h"
#include "private/qqmlinstruction_p.h"
#include "private/qv4instruction_p.h"
#include "private/qpointervaluepair_p.h"

QT_BEGIN_NAMESPACE

struct QV4Program;
class QV4Bindings : public QQmlAbstractExpression, 
                               public QQmlRefCount
{
    Q_DECLARE_TR_FUNCTIONS(QV4Bindings)
public:
    QV4Bindings(const char *program, QQmlContextData *context);
    virtual ~QV4Bindings();

    QQmlAbstractBinding *configBinding(QObject *target, QObject *scope,
                                       const QQmlInstruction::instr_assignV4Binding *);

#ifdef QML_THREADED_INTERPRETER
    static void **getDecodeInstrTable();
#endif

    struct Binding : public QQmlAbstractBinding, public QQmlDelayedError {
        Binding()
            : QQmlAbstractBinding(V4), target(0), scope(0), instruction(0), executedBlocks(0), parent(0) {}

        // Inherited from QQmlAbstractBinding
        static void destroy(QQmlAbstractBinding *, QQmlAbstractBinding::DestroyMode mode);
        static int propertyIndex(const QQmlAbstractBinding *);
        static QObject *object(const QQmlAbstractBinding *);
        static void setEnabled(QQmlAbstractBinding *, bool, QQmlPropertyPrivate::WriteFlags);
        static void update(QQmlAbstractBinding *, QQmlPropertyPrivate::WriteFlags);
        static void retargetBinding(QQmlAbstractBinding *, QObject *, int);

        void disconnect();

        void dump();

        struct Retarget {
            QObject *target;
            int targetProperty;
        };

        QPointerValuePair<QObject, Retarget> target;
        QObject *scope;

        // To save memory, we store flags inside the instruction pointer.
        //    instruction.flag1: enabled
        //    instruction.flag2: updating
        QFlagPointer<const QQmlInstruction::instr_assignV4Binding> instruction;

        quint32 executedBlocks;
        QV4Bindings *parent;

        inline bool enabledFlag() const { return instruction.flag(); }
        inline void setEnabledFlag(bool v) { instruction.setFlagValue(v); }
        inline bool updatingFlag() const { return instruction.flag2(); }
        inline void setUpdatingFlag(bool v) { instruction.setFlag2Value(v); }
    };

private:
    Q_DISABLE_COPY(QV4Bindings)

    class Subscription : public QQmlNotifierEndpoint
    {
    public:
        inline Subscription();

        // Index of this Subscription into the QV4Bindings::subscriptions array.
        // This may not be used before setBindings() was called.
        inline int method() const;

        inline void setBindings(QV4Bindings *bindings);
        inline QV4Bindings *bindings() const;

        inline bool active() const;
        inline void setActive(bool active);

        // Pointer to the parent QV4Bindings. The flag is used as the 'active' value.
        QFlagPointer<QV4Bindings> m_bindings;
    };
    friend void QV4BindingsSubscription_callback(QQmlNotifierEndpoint *e, void **);

    Subscription *subscriptions;

    void subscriptionNotify(int);
    void run(Binding *, QQmlPropertyPrivate::WriteFlags flags);

    QV4Program *program;
    Binding *bindings;

    void init();
    void run(int instr, quint32 &executedBlocks, QQmlContextData *context,
             QQmlDelayedError *error, QObject *scope, QObject *output, 
             QQmlPropertyPrivate::WriteFlags storeFlags,
             bool *invalidated
#ifdef QML_THREADED_INTERPRETER
             , void ***decode_instr = 0
#endif
             );


    inline void subscribeId(QQmlContextData *p, int idIndex, int subIndex);
    inline void subscribe(QObject *o, int notifyIndex, int subIndex, QQmlEngine *);

    inline static qint32 toInt32(double n);
    static const double D32;
    static quint32 toUint32(double n);

};

QT_END_NAMESPACE

#endif // QV4BINDINGS_P_H

