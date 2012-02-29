/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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

#include "private/qdeclarativeexpression_p.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qv4instruction_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

struct QV4Program;
class QV4Bindings : public QDeclarativeAbstractExpression, 
                               public QDeclarativeRefCount
{
    Q_DECLARE_TR_FUNCTIONS(QV4Bindings)
public:
    QV4Bindings(const char *program, QDeclarativeContextData *context,
                           QDeclarativeRefCount *);
    virtual ~QV4Bindings();

    QDeclarativeAbstractBinding *configBinding(int index, QObject *target, 
                                               QObject *scope, int property,
                                               int line, int column);

#ifdef QML_THREADED_INTERPRETER
    static void **getDecodeInstrTable();
#endif

private:
    Q_DISABLE_COPY(QV4Bindings)

    struct Binding : public QDeclarativeAbstractBinding, public QDeclarativeDelayedError {
        Binding() : enabled(false), updating(0), property(0),
                    scope(0), target(0), executedBlocks(0), parent(0) {}

        // Inherited from QDeclarativeAbstractBinding
        virtual void setEnabled(bool, QDeclarativePropertyPrivate::WriteFlags flags);
        virtual void update(QDeclarativePropertyPrivate::WriteFlags flags);
        virtual void destroy();
        virtual int propertyIndex() const;
        virtual QObject *object() const;

        int index:30;
        bool enabled:1;
        bool updating:1;
        // Encoding of property is coreIndex | (propType << 16) | (valueTypeIndex << 24)
        // propType and valueTypeIndex are only set if the property is a value type property
        int property;
        QObject *scope;
        int line;
        int column;
        QObject *target;
        quint32 executedBlocks;

        QV4Bindings *parent;
    };

    class Subscription : public QDeclarativeNotifierEndpoint
    {
    public:
        Subscription() : bindings(0), method(-1) { callback = &subscriptionCallback; }
        static void subscriptionCallback(QDeclarativeNotifierEndpoint *e);
        QV4Bindings *bindings;
        int method;
    };
    friend class Subscription;

    Subscription *subscriptions;

    void subscriptionNotify(int);
    void run(Binding *, QDeclarativePropertyPrivate::WriteFlags flags);

    QV4Program *program;
    QDeclarativeRefCount *dataRef;
    Binding *bindings;

    void init();
    void run(int instr, quint32 &executedBlocks, QDeclarativeContextData *context,
             QDeclarativeDelayedError *error, QObject *scope, QObject *output, 
             QDeclarativePropertyPrivate::WriteFlags storeFlags
#ifdef QML_THREADED_INTERPRETER
             , void ***decode_instr = 0
#endif
             );


    inline void unsubscribe(int subIndex);
    inline void subscribeId(QDeclarativeContextData *p, int idIndex, int subIndex);
    inline void subscribe(QObject *o, int notifyIndex, int subIndex);

    inline static qint32 toInt32(qreal n);
    static const qreal D32;
    static quint32 toUint32(qreal n);

};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QV4BINDINGS_P_H

