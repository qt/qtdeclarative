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
#include "private/qv4instruction_p.h"
#include "private/qpointervaluepair_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

struct QV4Program;
class QV4Bindings : public QQmlAbstractExpression, 
                               public QQmlRefCount
{
    Q_DECLARE_TR_FUNCTIONS(QV4Bindings)
public:
    QV4Bindings(const char *program, QQmlContextData *context);
    virtual ~QV4Bindings();

    QQmlAbstractBinding *configBinding(int index, int fallbackIndex, QObject *target,
                                       QObject *scope, int property, int propType,
                                       int line, int column);

#ifdef QML_THREADED_INTERPRETER
    static void **getDecodeInstrTable();
#endif

    struct Binding : public QQmlAbstractBinding, public QQmlDelayedError {
        Binding() : QQmlAbstractBinding(V4), index(-1), fallbackIndex(-1), enabled(false),
                    updating(0), property(0), propType(0), scope(0), target(0), executedBlocks(0), parent(0) {}

        // Inherited from QQmlAbstractBinding
        static void destroy(QQmlAbstractBinding *);
        static int propertyIndex(const QQmlAbstractBinding *);
        static QObject *object(const QQmlAbstractBinding *);
        static void setEnabled(QQmlAbstractBinding *, bool, QQmlPropertyPrivate::WriteFlags);
        static void update(QQmlAbstractBinding *, QQmlPropertyPrivate::WriteFlags);
        static void retargetBinding(QQmlAbstractBinding *, QObject *, int);

        struct Retarget {
            QObject *target;
            int targetProperty;
        };

        int index:15;
        int fallbackIndex:15;
        bool enabled:1;
        bool updating:1;

        // Encoding of property is: coreIndex | (valueTypeIndex << 16)
        // propType and valueTypeIndex are only set if the property is a value type property
        int property;
        quint16 propType;

        QObject *scope;
        int line;
        int column;
        QPointerValuePair<QObject, Retarget> target;
        quint32 executedBlocks;

        QV4Bindings *parent;
    };

private:
    Q_DISABLE_COPY(QV4Bindings)

    class Subscription : public QQmlNotifierEndpoint
    {
    public:
        inline Subscription();
        QV4Bindings *bindings;
        int method;
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


    inline void unsubscribe(int subIndex);
    inline void subscribeId(QQmlContextData *p, int idIndex, int subIndex);
    inline void subscribe(QObject *o, int notifyIndex, int subIndex, QQmlEngine *);

    inline static qint32 toInt32(double n);
    static const double D32;
    static quint32 toUint32(double n);

};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QV4BINDINGS_P_H

