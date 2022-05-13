// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTHREAD_P_H
#define QQMLTHREAD_P_H

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
class QMutex;

class QQmlThreadPrivate;
class QQmlThread
{
public:
    QQmlThread();
    virtual ~QQmlThread();

    void startup();
    void shutdown();
    bool isShutdown() const;

    QMutex &mutex();
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

    void waitForNextMessage();

protected:
    virtual void startupThread();
    virtual void shutdownThread();

private:
    friend class QQmlThreadPrivate;

    struct Message {
        Message() : next(nullptr) {}
        virtual ~Message() {}
        Message *next;
        virtual void call(QQmlThread *) = 0;
    };
    void internalCallMethodInThread(Message *);
    void internalCallMethodInMain(Message *);
    void internalPostMethodToThread(Message *);
    void internalPostMethodToMain(Message *);
    QQmlThreadPrivate *d;
};

template<class O>
void QQmlThread::callMethodInThread(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalCallMethodInThread(new I(Member));
}

template<typename T, class V, class O>
void QQmlThread::callMethodInThread(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalCallMethodInThread(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QQmlThread::callMethodInThread(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalCallMethodInThread(new I(Member, arg, arg2));
}

template<class O>
void QQmlThread::callMethodInMain(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalCallMethodInMain(new I(Member));
}

template<typename T, class V, class O>
void QQmlThread::callMethodInMain(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalCallMethodInMain(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QQmlThread::callMethodInMain(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalCallMethodInMain(new I(Member, arg, arg2));
}

template<class O>
void QQmlThread::postMethodToThread(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalPostMethodToThread(new I(Member));
}

template<typename T, class V, class O>
void QQmlThread::postMethodToThread(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalPostMethodToThread(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QQmlThread::postMethodToThread(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalPostMethodToThread(new I(Member, arg, arg2));
}

template<class O>
void QQmlThread::postMethodToMain(void (O::*Member)())
{
    struct I : public Message {
        void (O::*Member)();
        I(void (O::*Member)()) : Member(Member) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)();
        }
    };
    internalPostMethodToMain(new I(Member));
}

template<typename T, class V, class O>
void QQmlThread::postMethodToMain(void (O::*Member)(V), const T &arg)
{
    struct I : public Message {
        void (O::*Member)(V);
        T arg;
        I(void (O::*Member)(V), const T &arg) : Member(Member), arg(arg) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg);
        }
    };
    internalPostMethodToMain(new I(Member, arg));
}

template<typename T, typename T2, class V, class V2, class O>
void QQmlThread::postMethodToMain(void (O::*Member)(V, V2), const T &arg, const T2 &arg2)
{
    struct I : public Message {
        void (O::*Member)(V, V2);
        T arg;
        T2 arg2;
        I(void (O::*Member)(V, V2), const T &arg, const T2 &arg2) : Member(Member), arg(arg), arg2(arg2) {}
        void call(QQmlThread *thread) override {
            O *me = static_cast<O *>(thread);
            (me->*Member)(arg, arg2);
        }
    };
    internalPostMethodToMain(new I(Member, arg, arg2));
}

QT_END_NAMESPACE

#endif // QQMLTHREAD_P_H
