/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEVME_P_H
#define QDECLARATIVEVME_P_H

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

#include "qdeclarativeerror.h"
#include <private/qbitfield_p.h>
#include "qdeclarativeinstruction_p.h"
#include <private/qrecursionwatcher_p.h>

#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qcoreapplication.h>

#include <private/qv8_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qfinitestack_p.h>

#include <private/qdeclarativetrace_p.h>

QT_BEGIN_NAMESPACE

class QObject;
class QJSValue;
class QDeclarativeScriptData;
class QDeclarativeCompiledData;
class QDeclarativeCompiledData;
class QDeclarativeContextData;

namespace QDeclarativeVMETypes {
    struct List
    {
        List() : type(0) {}
        List(int t) : type(t) {}

        int type;
        QDeclarativeListProperty<void> qListProperty;
    };
}
Q_DECLARE_TYPEINFO(QDeclarativeVMETypes::List, Q_PRIMITIVE_TYPE  | Q_MOVABLE_TYPE);

class QDeclarativeVME
{
    Q_DECLARE_TR_FUNCTIONS(QDeclarativeVME)
public:
    class Interrupt {
    public:
        inline Interrupt();
        inline Interrupt(bool *runWhile);
        inline Interrupt(int nsecs);

        inline void reset();
        inline bool shouldInterrupt() const;
    private:
        enum Mode { None, Time, Flag };
        Mode mode;
        union {
            struct {
                QElapsedTimer timer;
                int nsecs;
            };
            bool *runWhile;
        };
    };

    QDeclarativeVME() : data(0), componentAttached(0) {}
    QDeclarativeVME(void *data) : data(data), componentAttached(0) {}

    void *data;
    QDeclarativeComponentAttached *componentAttached;
    QList<QDeclarativeEnginePrivate::FinalizeCallback> finalizeCallbacks;

    void init(QDeclarativeContextData *, QDeclarativeCompiledData *, int start,
              QDeclarativeContextData * = 0);
    bool initDeferred(QObject *);
    void reset();

    QObject *execute(QList<QDeclarativeError> *errors, const Interrupt & = Interrupt());
    QDeclarativeContextData *complete(const Interrupt & = Interrupt());

private:
    friend class QDeclarativeVMEGuard;

    QObject *run(QList<QDeclarativeError> *errors, const Interrupt &
#ifdef QML_THREADED_VME_INTERPRETER
                 , void ***storeJumpTable = 0
#endif
                );
    v8::Persistent<v8::Object> run(QDeclarativeContextData *, QDeclarativeScriptData *);

#ifdef QML_THREADED_VME_INTERPRETER
    static void **instructionJumpTable();
    friend class QDeclarativeCompiledData;
#endif

    QDeclarativeEngine *engine;
    QRecursionNode recursion;

#ifdef QML_ENABLE_TRACE
    QDeclarativeCompiledData *rootComponent;
#endif

    QFiniteStack<QObject *> objects;
    QFiniteStack<QDeclarativeVMETypes::List> lists;

    QFiniteStack<QDeclarativeAbstractBinding *> bindValues;
    QFiniteStack<QDeclarativeParserStatus *> parserStatus;
#ifdef QML_ENABLE_TRACE
    QFiniteStack<QDeclarativeData *> parserStatusData;
#endif

    QDeclarativeGuardedContextData rootContext;
    QDeclarativeGuardedContextData creationContext;

    struct State {
        enum Flag { Deferred = 0x00000001 };

        State() : flags(0), context(0), compiledData(0), instructionStream(0) {}
        quint32 flags;
        QDeclarativeContextData *context;
        QDeclarativeCompiledData *compiledData;
        const char *instructionStream;
        QBitField bindingSkipList;
    };

    QStack<State> states;

    static void blank(QFiniteStack<QDeclarativeParserStatus *> &);
    static void blank(QFiniteStack<QDeclarativeAbstractBinding *> &);
};

// Used to check that a QDeclarativeVME that is interrupted mid-execution
// is still valid.  Checks all the objects and contexts have not been 
// deleted.
class QDeclarativeVMEGuard
{
public:
    QDeclarativeVMEGuard();
    ~QDeclarativeVMEGuard();

    void guard(QDeclarativeVME *);
    void clear();

    bool isOK() const;

private:
    int m_objectCount;
    QDeclarativeGuard<QObject> *m_objects;
    int m_contextCount;
    QDeclarativeGuardedContextData *m_contexts;
};

QDeclarativeVME::Interrupt::Interrupt()
: mode(None)
{
}

QDeclarativeVME::Interrupt::Interrupt(bool *runWhile)
: mode(Flag), runWhile(runWhile)
{
}

QDeclarativeVME::Interrupt::Interrupt(int nsecs)
: mode(Time), nsecs(nsecs)
{
}

void QDeclarativeVME::Interrupt::reset()
{
    if (mode == Time) 
        timer.start();
}

bool QDeclarativeVME::Interrupt::shouldInterrupt() const
{
    if (mode == None) {
        return false;
    } else if (mode == Time) {
        return timer.nsecsElapsed() > nsecs;
    } else if (mode == Flag) {
        return !*runWhile;
    } else {
        return false;
    }
}

QT_END_NAMESPACE

#endif // QDECLARATIVEVME_P_H
