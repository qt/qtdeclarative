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

#ifndef QV4QOBJECTWRAPPER_P_H
#define QV4QOBJECTWRAPPER_P_H

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
#include <QtCore/qmetatype.h>
#include <QtCore/qpair.h>
#include <QtCore/qhash.h>
#include <private/qhashedstring_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qintrusivelist_p.h>

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>

QT_BEGIN_NAMESPACE

class QObject;
class QQmlData;
class QQmlPropertyCache;
class QQmlPropertyData;

namespace QV4 {
struct QObjectSlotDispatcher;

struct Q_QML_EXPORT QObjectWrapper : public QV4::Object
{
    Q_MANAGED

    enum RevisionMode { IgnoreRevision, CheckRevision };

    static void initializeBindings(ExecutionEngine *engine);

    QObject *object() const { return m_object.data(); }

    Value getQmlProperty(ExecutionContext *ctx, QQmlContextData *qmlContext, String *name, RevisionMode revisionMode, bool *hasProperty = 0, bool includeImports = false);
    static Value getQmlProperty(ExecutionContext *ctx, QQmlContextData *qmlContext, QObject *object, String *name, RevisionMode revisionMode, bool *hasProperty = 0);

    static bool setQmlProperty(ExecutionContext *ctx, QQmlContextData *qmlContext, QObject *object, String *name, RevisionMode revisionMode, const Value &value);

    static Value wrap(ExecutionEngine *engine, QObject *object);

    using Object::get;

private:
    static Value create(ExecutionEngine *engine, QQmlData *ddata, QObject *object);

    QObjectWrapper(ExecutionEngine *engine, QObject *object);

    QQmlPropertyData *findProperty(ExecutionEngine *engine, QQmlContextData *qmlContext, String *name, RevisionMode revisionMode, QQmlPropertyData *local) const;

    QPointer<QObject> m_object;
    String *m_destroy;
    String *m_toString;

    static Value get(Managed *m, String *name, bool *hasProperty);
    static void put(Managed *m, String *name, const Value &value);
    static PropertyAttributes query(const Managed *, String *name);
    static Property *advanceIterator(Managed *m, ObjectIterator *it, String **name, uint *index, PropertyAttributes *attributes);
    static void markObjects(Managed *that);
    static void collectDeletables(Managed *m, GCDeletable **deletable);
    static void destroy(Managed *that)
    {
        static_cast<QObjectWrapper *>(that)->~QObjectWrapper();
    }

    static Value method_connect(SimpleCallContext *ctx);
    static Value method_disconnect(SimpleCallContext *ctx);
};

struct QObjectMethod : public QV4::FunctionObject
{
    Q_MANAGED

    enum { DestroyMethod = -1, ToStringMethod = -2 };

    static Value create(QV4::ExecutionContext *scope, QObject *object, int index, const Value &qmlGlobal = Value::undefinedValue());

    int methodIndex() const { return m_index; }
    QObject *object() const { return m_object.data(); }

private:
    QObjectMethod(QV4::ExecutionContext *scope, QObject *object, int index, const QV4::Value &qmlGlobal);

    QV4::Value method_toString(QV4::ExecutionContext *ctx);
    QV4::Value method_destroy(QV4::ExecutionContext *ctx, const Value *args, int argc);

    QPointer<QObject> m_object;
    int m_index;
    QV4::PersistentValue m_qmlGlobal;

    static Value call(Managed *, CallData *callData);

    Value callInternal(CallData *callData);

    static void destroy(Managed *that)
    {
        static_cast<QObjectMethod *>(that)->~QObjectMethod();
    }
};

struct QmlSignalHandler : public QV4::Object
{
    Q_MANAGED

    QmlSignalHandler(ExecutionEngine *engine, QObject *object, int signalIndex);

    int signalIndex() const { return m_signalIndex; }
    QObject *object() const { return m_object.data(); }

private:
    QPointer<QObject> m_object;
    int m_signalIndex;

    static void destroy(Managed *that)
    {
        static_cast<QmlSignalHandler *>(that)->~QmlSignalHandler();
    }
};

class MultiplyWrappedQObjectMap : public QObject,
                                  private QHash<QObject*, Object*>
{
    Q_OBJECT
public:
    typedef QHash<QObject*, Object*>::ConstIterator ConstIterator;
    typedef QHash<QObject*, Object*>::Iterator Iterator;

    ConstIterator begin() const { return QHash<QObject*, Object*>::constBegin(); }
    Iterator begin() { return QHash<QObject*, Object*>::begin(); }
    ConstIterator end() const { return QHash<QObject*, Object*>::constEnd(); }
    Iterator end() { return QHash<QObject*, Object*>::end(); }

    void insert(QObject *key, Object *value);
    Object *value(QObject *key) const { return QHash<QObject*, Object*>::value(key, 0); }
    Iterator erase(Iterator it);
    void remove(QObject *key);

private Q_SLOTS:
    void removeDestroyedObject(QObject*);
};

}

QT_END_NAMESPACE

#endif // QV4QOBJECTWRAPPER_P_H


