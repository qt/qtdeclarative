/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVETHREAD_P_H
#define QDECLARATIVETHREAD_P_H

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


#include <QtCore/qglobal.h>

#include <private/qintrusivelist_p.h>

QT_BEGIN_NAMESPACE

class QThread;

class QDeclarativeThreadPrivate;
class QDeclarativeThread 
{
public:
    QDeclarativeThread();
    virtual ~QDeclarativeThread();
    void shutdown();

    void lock();
    void unlock();
    void wakeOne();
    void wakeAll();
    void wait();

    QThread *thread() const;
    bool isThisThread() const;

    // Synchronously invoke a method in the thread
    template<class O>
    inline void callMethodInThread(void (O::*Member)());
    template<typename T, class V, class O>
    inline void callMethodInThread(void (O::*Member)(V), const T &);
    template<typename T, typename T2, class V, class V2, class O>
    inline void callMethodInThread(void (O::*Member)(V, V2), const T &, const T2 &);

    // Synchronously invoke a method in the main thread.  If the main thread is
    // blocked in a callMethodInThread() call, the call is made from within that
    // call.
    template<class O>
    inline void callMethodInMain(void (O::*Member)());
    template<typename T, class V, class O>
    inline void callMethodInMain(void (O::*Member)(V), const T &);
    template<typename T, typename T2, class V, class V2, class O>
    inline void callMethodInMain(void (O::*Member)(V, V2), const T &, const T2 &);

    // Asynchronously invoke a method in the thread.
    template<class O>
    inline void postMethodToThread(void (O::*Member)());
    template<typename T, class V, class O>
    inline void postMethodToThread(void (O::*Member)(V), const T &);
    template<typename T, typename T2, class V, class V2, class O>
    inline void postMethodToThread(void (O::*Member)(V, V2), const T &, const T2 &);

    // Asynchronously invoke a method in the main thread.
    template<class O>
    inline void postMethodToMain(void (O::*Member)());
    template<typename T, class V, class O>
    inline void postMethodToMain(void (O::*Member)(V), const T &);
    template<typename T, typename T2, class V, class V2, class O>
    inline void postMethodToMain(void (O::*Member)(V, V2), const T &, const T2 &);

protected:
    virtual void startupThread();
    virtual void shutdownThread();

private:
    friend class QDeclarativeThreadPrivate;

    struct Message {
        Message() : next(0) {}
        virtual ~Message() {}
        Message *next;
        virtual void call(QDeclarativeThread *) = 0;
    };
    void internalCallMethodInThread(Message *);
    void internalCallMethodInMain(Message *);
    void internalPostMethodToThread(Message *);
    void internalPostMethodToMain(Message *);
    QDeclarativeThreadPrivate *d;
};

template<class O>
void QDeclarativeThread::callMethodInThread(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalCallMethodInThread(new I(Member));
}

template<typename T, class V, class O>
void QDeclarativeThread::callMethodInThread(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalCallMethodInThread(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QDeclarativeThread::callMethodInThread(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalCallMethodInThread(new I(Member, arg, arg2));
}

template<class O>
void QDeclarativeThread::callMethodInMain(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalCallMethodInMain(new I(Member));
}

template<typename T, class V, class O>
void QDeclarativeThread::callMethodInMain(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalCallMethodInMain(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QDeclarativeThread::callMethodInMain(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalCallMethodInMain(new I(Member, arg, arg2));
}

template<class O>
void QDeclarativeThread::postMethodToThread(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalPostMethodToThread(new I(Member));
}

template<typename T, class V, class O>
void QDeclarativeThread::postMethodToThread(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalPostMethodToThread(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QDeclarativeThread::postMethodToThread(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalPostMethodToThread(new I(Member, arg, arg2));
}

template<class O>
void QDeclarativeThread::postMethodToMain(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalPostMethodToMain(new I(Member));
}

template<typename T, class V, class O>
void QDeclarativeThread::postMethodToMain(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalPostMethodToMain(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QDeclarativeThread::postMethodToMain(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        virtual void call(QDeclarativeThread *thread) {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalPostMethodToMain(new I(Member, arg, arg2));
}

QT_END_NAMESPACE

#endif // QDECLARATIVETHREAD_P_H
