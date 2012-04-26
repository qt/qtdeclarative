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

#ifndef QQMLDATA_P_H
#define QQMLDATA_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qobject_p.h>
#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

template <class Key, class T> class QHash;
class QQmlGuardImpl;
class QQmlCompiledData;
class QQmlAbstractBinding;
class QQmlAbstractBoundSignal;
class QQmlContext;
class QQmlPropertyCache;
class QQmlContextData;
class QQmlNotifier;
class QQmlDataExtended;
class QQmlNotifierEndpoint;
// This class is structured in such a way, that simply zero'ing it is the
// default state for elemental object allocations.  This is crucial in the
// workings of the QQmlInstruction::CreateSimpleObject instruction.
// Don't change anything here without first considering that case!
class Q_QML_EXPORT QQmlData : public QAbstractDeclarativeData
{
public:
    QQmlData()
        : ownMemory(true), ownContext(false), indestructible(true), explicitIndestructibleSet(false), 
          hasTaintedV8Object(false), isQueuedForDeletion(false), inCreation(false), notifyList(0), context(0), outerContext(0),
          bindings(0), signalHandlers(0), nextContextObject(0), prevContextObject(0), bindingBitsSize(0), bindingBits(0),
          lineNumber(0), columnNumber(0), deferredComponent(0), deferredIdx(0), v8objectid(0), 
          propertyCache(0), guards(0), extendedData(0) {
        init();
    }

    static inline void init() {
        static bool initialized = false;
        if (!initialized) {
            initialized = true;
            QAbstractDeclarativeData::destroyed = destroyed;
            QAbstractDeclarativeData::parentChanged = parentChanged;
            QAbstractDeclarativeData::objectNameChanged = objectNameChanged;
            QAbstractDeclarativeData::signalEmitted = signalEmitted;
            QAbstractDeclarativeData::receivers = receivers;
        }
    }

    static void destroyed(QAbstractDeclarativeData *, QObject *);
    static void parentChanged(QAbstractDeclarativeData *, QObject *, QObject *);
    static void objectNameChanged(QAbstractDeclarativeData *, QObject *);
    static void signalEmitted(QAbstractDeclarativeData *, QObject *, int, void **);
    static int receivers(QAbstractDeclarativeData *, const QObject *, int);

    void destroyed(QObject *);
    void parentChanged(QObject *, QObject *);
    void objectNameChanged(QObject *);

    void setImplicitDestructible() {
        if (!explicitIndestructibleSet) indestructible = false;
    }

    quint32 ownMemory:1;
    quint32 ownContext:1;
    quint32 indestructible:1;
    quint32 explicitIndestructibleSet:1;
    quint32 hasTaintedV8Object:1;
    quint32 isQueuedForDeletion:1;
    /*
     * inCreation should be true only when creating top level CPP and QML objects,
     * v8 GC will check this flag, only deletes the objects when inCreation is false.
     */
    quint32 inCreation:1;
    quint32 dummy:25;

    struct NotifyList {
        quint64 connectionMask;

        quint16 maximumTodoIndex;
        quint16 notifiesSize;

        QQmlNotifierEndpoint *todo;
        QQmlNotifierEndpoint**notifies;
        void layout();
    private:
        void layout(QQmlNotifierEndpoint*);
    };
    NotifyList *notifyList;
    
    inline QQmlNotifierEndpoint *notify(int index);
    void addNotify(int index, QQmlNotifierEndpoint *);
    int endpointCount(int index);
    bool signalHasEndpoint(int index);

    // The context that created the C++ object
    QQmlContextData *context; 
    // The outermost context in which this object lives
    QQmlContextData *outerContext;

    QQmlAbstractBinding *bindings;
    QQmlAbstractBoundSignal *signalHandlers;

    // Linked list for QQmlContext::contextObjects
    QQmlData *nextContextObject;
    QQmlData**prevContextObject;

    int bindingBitsSize;
    quint32 *bindingBits; 
    bool hasBindingBit(int) const;
    void clearBindingBit(int);
    void setBindingBit(QObject *obj, int);

    ushort lineNumber;
    ushort columnNumber;

    QQmlCompiledData *deferredComponent; // Can't this be found from the context?
    unsigned int deferredIdx;

    quint32 v8objectid;
    v8::Persistent<v8::Object> v8object;

    QQmlPropertyCache *propertyCache;

    QQmlGuardImpl *guards;

    static QQmlData *get(const QObject *object, bool create = false) {
        QObjectPrivate *priv = QObjectPrivate::get(const_cast<QObject *>(object));
        if (priv->wasDeleted) {
            Q_ASSERT(!create);
            return 0;
        } else if (priv->declarativeData) {
            return static_cast<QQmlData *>(priv->declarativeData);
        } else if (create) {
            priv->declarativeData = new QQmlData;
            return static_cast<QQmlData *>(priv->declarativeData);
        } else {
            return 0;
        }
    }

    bool hasExtendedData() const { return extendedData != 0; }
    QQmlNotifier *objectNameNotifier() const;
    QHash<int, QObject *> *attachedProperties() const;

    static inline bool wasDeleted(QObject *);

    static void markAsDeleted(QObject *);
    static inline void setQueuedForDeletion(QObject *);

private:
    // For objectNameNotifier and attachedProperties
    mutable QQmlDataExtended *extendedData;
};

bool QQmlData::wasDeleted(QObject *object)
{
    if (!object)
        return true;

    QObjectPrivate *priv = QObjectPrivate::get(object);
    if (priv->wasDeleted)
        return true;

    return priv->declarativeData &&
           static_cast<QQmlData *>(priv->declarativeData)->isQueuedForDeletion;
}

void QQmlData::setQueuedForDeletion(QObject *object)
{
    if (object) {
        if (QObjectPrivate *priv = QObjectPrivate::get(object)) {
            if (!priv->wasDeleted && priv->declarativeData) {
                static_cast<QQmlData *>(priv->declarativeData)->isQueuedForDeletion = true;
            }
        }
    }
}

QQmlNotifierEndpoint *QQmlData::notify(int index)
{
    Q_ASSERT(index <= 0xFFFF);

    if (!notifyList || !(notifyList->connectionMask & (1ULL << quint64(index % 64)))) {
        return 0;
    } else if (index < notifyList->notifiesSize) {
        return notifyList->notifies[index];
    } else if (index <= notifyList->maximumTodoIndex) {
        notifyList->layout();
    }

    if (index < notifyList->notifiesSize) {
        return notifyList->notifies[index];
    } else {
        return 0;
    }
}

QT_END_NAMESPACE

#endif // QQMLDATA_P_H
