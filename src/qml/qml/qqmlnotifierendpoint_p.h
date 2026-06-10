// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLNOTIFIERENDPOINT_P_H
#define QQMLNOTIFIERENDPOINT_P_H

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

#include <private/qmetaobject_p.h>
#include <private/qobject_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qthread_p.h>
#include <private/qtqmlglobal_p.h>

#include <QtCore/qmetaobject.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

class QQmlNotifier;
class QQmlNotifyList;

class QQmlNotifierEndpoint
{
    QQmlNotifierEndpoint *next;
    QQmlNotifierEndpoint **prev;

public:
    // QQmlNotifierEndpoint can only invoke one of a set of pre-defined callbacks.
    // To add another callback, extend this enum and add the callback to the top
    // of qqmlnotifier.cpp.  Four bits are reserved for the callback, so there can
    // be up to 15 of them (0 is reserved).
    enum Callback {
        None = 0,
        QQmlBoundSignal = 1,
        QQmlJavaScriptExpressionGuard = 2,
        QQmlVMEMetaObjectEndpoint = 3,
        QQmlUnbindableToUnbindableGuard = 4,
        QQmlUnbindableToBindableGuard = 5,
        QQmlDirtyReferenceObject = 6,
    };

    inline QQmlNotifierEndpoint(Callback callback);
    inline ~QQmlNotifierEndpoint();

    inline bool isConnected() const;
    inline bool isConnected(QObject *source, int sourceSignal) const;
    inline bool isConnected(QQmlNotifier *) const;

    inline void disconnect();

    inline bool isNotifying() const;
    inline void startNotifying(qintptr *originalSenderPtr);
    inline void stopNotifying(qintptr *originalSenderPtr);

    inline void cancelNotify();

    inline int signalIndex() const { return sourceSignal; }

    inline Callback callbackType() const { return callback; }
    inline QQmlNotifierEndpoint *nextEndpoint() const { return next; }

    inline qintptr sender() const;
    inline void setSender(qintptr sender);

    inline QObject *senderAsObject() const;
    inline QQmlNotifier *senderAsNotifier() const;

private:
    friend class QQmlData;
    friend class QQmlNotifier;
    friend class QQmlNotifyList;

    // Contains either the QObject*, or the QQmlNotifier* that this
    // endpoint is connected to.  While the endpoint is notifying, the
    // senderPtr points to another qintptr that contains this value.
    qintptr senderPtr;

    Callback callback : 4;
    int needsConnectNotify : 1;
    // The index is in the range returned by QObjectPrivate::signalIndex().
    // This is different from QMetaMethod::methodIndex().
    signed int sourceSignal : 27;
};

QQmlNotifierEndpoint::QQmlNotifierEndpoint(Callback callback)
    : next(nullptr),
      prev(nullptr),
      senderPtr(0),
      callback(callback),
      needsConnectNotify(false),
      sourceSignal(-1)
{
}

QQmlNotifierEndpoint::~QQmlNotifierEndpoint()
{
    disconnect();
}

bool QQmlNotifierEndpoint::isConnected() const
{
    return prev != nullptr;
}

/*! \internal
    \a sourceSignal MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
bool QQmlNotifierEndpoint::isConnected(QObject *source, int sourceSignal) const
{
    return this->sourceSignal != -1 && senderAsObject() == source
            && this->sourceSignal == sourceSignal;
}

bool QQmlNotifierEndpoint::isConnected(QQmlNotifier *notifier) const
{
    return sourceSignal == -1 && senderAsNotifier() == notifier;
}

inline void QQmlNotifierEndpoint::disconnect()
{
    // Remove from notifier chain before calling disconnectNotify(), so that that
    // QObject::receivers() returns the correct value in there
    if (next)
        next->prev = prev;
    if (prev)
        *prev = next;

    if (sourceSignal != -1 && needsConnectNotify) {
        QObject *const obj = senderAsObject();
        Q_ASSERT(obj);
        QObjectPrivate *const priv = QObjectPrivate::get(obj);

        // In some degenerate cases an object being destructed might be unable
        // to produce a metaObject(). Therefore we check here.
        if (const QMetaObject *mo = obj->metaObject())
            priv->disconnectNotify(QMetaObjectPrivate::signal(mo, sourceSignal));
    }

    setSender(0x0);
    next = nullptr;
    prev = nullptr;
    sourceSignal = -1;
}

/*!
Returns true if a notify is in progress.  This means that the signal or QQmlNotifier
that this endpoint is connected to has been triggered, but this endpoint's callback has not
yet been called.

An in progress notify can be cancelled by calling cancelNotify.
*/
inline bool QQmlNotifierEndpoint::isNotifying() const
{
    return senderPtr & 0x1;
}

inline void QQmlNotifierEndpoint::startNotifying(qintptr *originalSenderPtr)
{
    Q_ASSERT(*originalSenderPtr == 0);
    // Set the endpoint to notifying:
    // - Save the original senderPtr,
    *originalSenderPtr = senderPtr;
    // - Take a pointer of it,
    // - And assign that to the senderPtr, including a flag to signify "notifying".
    senderPtr = qintptr(originalSenderPtr) | 0x1;
}

inline void QQmlNotifierEndpoint::stopNotifying(qintptr *originalSenderPtr)
{
    // End of notifying, restore values
    Q_ASSERT((senderPtr & ~0x1) == qintptr(originalSenderPtr));
    senderPtr = *originalSenderPtr;
    *originalSenderPtr = 0;
}

/*!
Cancel any notifies that are in progress.
*/
inline void QQmlNotifierEndpoint::cancelNotify()
{
    if (isNotifying()) {
        auto *ptr = (qintptr *)(senderPtr & ~0x1);
        Q_ASSERT(ptr);
        senderPtr = *ptr;
        *ptr = 0;
    }
}

inline qintptr QQmlNotifierEndpoint::sender() const
{
    return isNotifying() ? *(qintptr *)(senderPtr & ~0x1) : senderPtr;
}

inline void QQmlNotifierEndpoint::setSender(qintptr sender)
{
    // If we're just notifying, we write through to the originalSenderPtr
    if (isNotifying())
        *(qintptr *)(senderPtr & ~0x1) = sender;
    else
        senderPtr = sender;
}

inline QObject *QQmlNotifierEndpoint::senderAsObject() const
{
    return (QObject *)(sender());
}

inline QQmlNotifier *QQmlNotifierEndpoint::senderAsNotifier() const
{
    return (QQmlNotifier *)(sender());
}

QT_END_NAMESPACE

#endif // QQMLNOTIFIERENDPOINT_P_H
