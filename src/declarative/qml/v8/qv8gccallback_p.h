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

#ifndef QV8GCCALLBACK_P_H
#define QV8GCCALLBACK_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qthreadstorage.h>
#include <private/qv8_p.h>
#include <private/qintrusivelist_p.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QV8GCCallback
{
private:
    class ThreadData;
public:
    static void garbageCollectorPrologueCallback(v8::GCType, v8::GCCallbackFlags);
    static void registerGcPrologueCallback();
    static void releaseWorkerThreadGcPrologueCallbackData();

    class Q_AUTOTEST_EXPORT Referencer {
    public:
        ~Referencer();
        void addRelationship(QObject *object, v8::Persistent<v8::Value> handle);
        void addRelationship(QObject *object, QObject *other);
        void dispose();
    private:
        Referencer();
        static v8::Persistent<v8::Object> *findOwnerAndStrength(QObject *qobjectOwner, bool *shouldBeStrong);
        v8::Persistent<v8::Object> strongReferencer;
        v8::Persistent<v8::Context> context;
        friend class QV8GCCallback::ThreadData;
    };

    class Q_AUTOTEST_EXPORT Node {
    public:
        typedef void (*PrologueCallback)(Referencer *r, Node *node);
        Node(PrologueCallback callback);
        ~Node();

        QIntrusiveListNode node;
        PrologueCallback prologueCallback;
    };

    static void addGcCallbackNode(Node *node);

private:
    class ThreadData {
    public:
        ThreadData() : gcPrologueCallbackRegistered(false) { }
        ~ThreadData();
        Referencer referencer;
        bool gcPrologueCallbackRegistered;
        QIntrusiveList<Node, &Node::node> gcCallbackNodes;
    };

    static void initializeThreadData();
    static QThreadStorage<ThreadData *> threadData;
};

QT_END_NAMESPACE

#endif // QV8GCCALLBACK_P_H

