// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlthread_p.h"

#include <private/qfieldlist_p.h>
#include <private/qtqmlglobal_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qcoreapplication.h>

QT_REQUIRE_CONFIG(qml_type_loader_thread);

QT_BEGIN_NAMESPACE

/*!
    \class QQmlThread
    \inmodule QtQml
    \internal
*/

class QQmlThreadPrivate : public QThread
{
public:
    struct ThreadObject : public QObject
    {
        ThreadObject(QQmlThreadPrivate *p);
        bool event(QEvent *e) override;
        QQmlThreadPrivate *p;
    };

    QQmlThreadPrivate(QQmlThread *);
    QQmlThread *q;

    inline void lock() { _mutex.lock(); }
    inline void unlock() { _mutex.unlock(); }
    inline void wait() { _wait.wait(&_mutex); }
    inline void wakeOne() { _wait.wakeOne(); }

    bool m_threadProcessing  = false; // Set when the thread is processing messages
    bool m_mainProcessing    = false; // Set when the main thread is processing messages
    bool m_mainThreadWaiting = false; // Set by main thread if it is waiting for the message queue to empty
    bool m_shutdown          = false; // Set by main thread to announce shutdown in progress

    typedef QFieldList<QQmlThread::Message, &QQmlThread::Message::next> MessageList;
    MessageList threadList;
    MessageList mainList;

    QQmlThread::Message *mainSync = nullptr;
    ThreadObject m_threadObject;

    void triggerMainEvent();
    void triggerThreadEvent();

    void mainEvent();
    void threadEvent();

protected:
    bool event(QEvent *) override;

private:
    QMutex _mutex;
    QWaitCondition _wait;
};

QQmlThreadPrivate::ThreadObject::ThreadObject(QQmlThreadPrivate *p)
: p(p)
{
}

// Trigger mainEvent in main thread.  Must be called from thread.
void QQmlThreadPrivate::triggerMainEvent()
{
    Q_ASSERT(q->isThisThread());
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

// Trigger even in thread.  Must be called from main thread.
void QQmlThreadPrivate::triggerThreadEvent()
{
    Q_ASSERT(!q->isThisThread());
    QCoreApplication::postEvent(&m_threadObject, new QEvent(QEvent::User));
}

bool QQmlThreadPrivate::ThreadObject::event(QEvent *e)
{
    if (e->type() == QEvent::User)
        p->threadEvent();
    return QObject::event(e);
}

QQmlThreadPrivate::QQmlThreadPrivate(QQmlThread *q) : q(q), m_threadObject(this)
{
    setObjectName(QStringLiteral("QQmlThread"));
    // This size is aligned with the recursion depth limits in the parser/codegen. In case of
    // absurd content we want to hit the recursion checks instead of running out of stack.
    setStackSize(8 * 1024 * 1024);
}

bool QQmlThreadPrivate::event(QEvent *e)
{
    if (e->type() == QEvent::User)
        mainEvent();
    return QThread::event(e);
}

void QQmlThreadPrivate::mainEvent()
{
    lock();

    m_mainProcessing = true;

    while (!mainList.isEmpty() || mainSync) {
        bool isSync = mainSync != nullptr;
        QQmlThread::Message *message = isSync?mainSync:mainList.takeFirst();
        unlock();

        message->call(q);
        delete message;

        lock();

        if (isSync) {
            mainSync = nullptr;
            wakeOne();
        }
    }

    m_mainProcessing = false;

    unlock();
}

void QQmlThreadPrivate::threadEvent()
{
    lock();

    for (;;) {
        if (!threadList.isEmpty()) {
            m_threadProcessing = true;

            QQmlThread::Message *message = threadList.first();

            unlock();

            message->call(q);

            lock();

            delete threadList.takeFirst();
        } else {
            wakeOne();

            m_threadProcessing = false;

            unlock();

            return;
        }
    }
}

QQmlThread::QQmlThread()
: d(new QQmlThreadPrivate(this))
{
}

QQmlThread::~QQmlThread()
{
    delete d;
}

/*!
    \internal
    Starts the actual worker thread.
 */
void QQmlThread::startup()
{
    Q_ASSERT(!d->m_shutdown);
    d->start();
    d->m_threadObject.moveToThread(d);
}

void QQmlThread::shutdown()
{
    d->lock();

    // The type loader thread may have multiple messages that ask for a callback into the main
    // thread. Simply deleting mainSync once is not enough to stop that. We need to explicitly
    // tell the type loader thread not to wait for the main thread anymore. Therefore m_shutdown.
    Q_ASSERT(!d->m_shutdown);
    d->m_shutdown = true;

    d->quit();

    if (d->mainSync)
        d->wakeOne();

    d->unlock();
    d->QThread::wait();

    d->m_shutdown = false;
}

void QQmlThread::lock()
{
    d->lock();
}

void QQmlThread::unlock()
{
    d->unlock();
}

void QQmlThread::wakeOne()
{
    d->wakeOne();
}

void QQmlThread::wait()
{
    d->wait();
}

bool QQmlThread::isThisThread() const
{
    return d->isCurrentThread();
}

bool QQmlThread::isParentThread() const
{
    // The thread() of the QQmlThread is its parent thread.
    return d->thread()->isCurrentThread();
}

QThread *QQmlThread::thread() const
{
    return const_cast<QThread *>(static_cast<const QThread *>(d));
}

/*!
    \internal
    An object living in the QML thread, in case you want to parent
    other objects to it.
*/
QObject *QQmlThread::threadObject() const
{
    return &d->m_threadObject;
}

void QQmlThread::internalCallMethodInThread(Message *message)
{
    Q_ASSERT(!isThisThread());
    d->lock();
    Q_ASSERT(d->m_mainThreadWaiting == false);

    bool wasEmpty = d->threadList.isEmpty();
    d->threadList.append(message);
    if (wasEmpty && d->m_threadProcessing == false)
        d->triggerThreadEvent();

    d->m_mainThreadWaiting = true;

    do {
        if (d->mainSync) {
            QQmlThread::Message *message = d->mainSync;
            unlock();
            message->call(this);
            delete message;
            lock();
            d->mainSync = nullptr;
            wakeOne();
        } else {
            d->wait();
        }
    } while (d->mainSync || !d->threadList.isEmpty());

    d->m_mainThreadWaiting = false;
    d->unlock();
}

/*!
    \internal
    \note This method needs to run in the worker/QQmlThread

    This runs \a message in the main thread, and blocks the
    worker thread until the call has completed
 */
void QQmlThread::internalCallMethodInMain(Message *message)
{
    Q_ASSERT(isThisThread());
    d->lock();

    Q_ASSERT(d->mainSync == nullptr);
    d->mainSync = message;

    if (d->m_mainThreadWaiting) {
        d->wakeOne();
    } else if (d->m_mainProcessing) {
        // Do nothing - it is already looping
    } else {
        d->triggerMainEvent();
    }

    while (d->mainSync) {
        if (d->m_shutdown) {
            delete d->mainSync;
            d->mainSync = nullptr;
            break;
        }
        d->wait();
    }

    d->unlock();
}

void QQmlThread::internalPostMethodToThread(Message *message)
{
    Q_ASSERT(!isThisThread());
    d->lock();
    bool wasEmpty = d->threadList.isEmpty();
    d->threadList.append(message);
    if (wasEmpty && d->m_threadProcessing == false)
        d->triggerThreadEvent();
    d->unlock();
}

void QQmlThread::internalPostMethodToMain(Message *message)
{
    Q_ASSERT(isThisThread());
    d->lock();
    bool wasEmpty = d->mainList.isEmpty();
    d->mainList.append(message);
    if (wasEmpty && d->m_mainProcessing == false)
        d->triggerMainEvent();
    d->unlock();
}

/*!
    \internal
    \note This method must be called in the main thread
    \warning This method requires that the lock is held!

    A call to this method will either:
    - run a message requested to run synchronously on the main thread if there is one
      (and return afterrwards),
    - wait for the worker thread to notify it if the worker thread has pending work,
    - or simply return if neither of the conditions above hold
 */
void QQmlThread::waitForNextMessage()
{
    Q_ASSERT(!isThisThread());
    Q_ASSERT(d->m_mainThreadWaiting == false);

    d->m_mainThreadWaiting = true;

    if (d->mainSync || !d->threadList.isEmpty()) {
        if (d->mainSync) {
            QQmlThread::Message *message = d->mainSync;
            unlock();
            message->call(this);
            delete message;
            lock();
            d->mainSync = nullptr;
            wakeOne();
        } else {
            d->wait();
        }
    }

    d->m_mainThreadWaiting = false;
}

/*!
    \internal
    \note This method must be called in the main thread
    \warning This method requires that the lock is held!

    Clear all pending events, for either thread.
*/
void QQmlThread::discardMessages()
{
    Q_ASSERT(!isThisThread());
    if (Message *mainSync = std::exchange(d->mainSync, nullptr))
        delete mainSync;
    while (!d->mainList.isEmpty())
        delete d->mainList.takeFirst();
    while (!d->threadList.isEmpty())
        delete d->threadList.takeFirst();
}

QT_END_NAMESPACE
