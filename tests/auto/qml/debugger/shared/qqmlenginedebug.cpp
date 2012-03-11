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

#include "qqmlenginedebug_p.h"

#include "qqmldebugclient.h"

#include <private/qqmlenginedebugservice_p.h>

#include <QtCore/private/qobject_p.h>

class QQmlEngineDebugClient : public QQmlDebugClient
{
public:
    QQmlEngineDebugClient(QQmlDebugConnection *client, QQmlEngineDebugPrivate *p);

protected:
    virtual void stateChanged(State state);
    virtual void messageReceived(const QByteArray &);

private:
    QQmlEngineDebugPrivate *priv;
    friend class QQmlEngineDebugPrivate;
};

class QQmlEngineDebugPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlEngineDebug)
public:
    QQmlEngineDebugPrivate(QQmlDebugConnection *);
    ~QQmlEngineDebugPrivate();

    void stateChanged(QQmlEngineDebug::State status);
    void message(const QByteArray &);

    QQmlEngineDebugClient *client;
    int nextId;
    int getId();

    void decode(QDataStream &, QQmlDebugContextReference &);
    void decode(QDataStream &, QQmlDebugObjectReference &, bool simple);

    static void remove(QQmlEngineDebug *, QQmlDebugEnginesQuery *);
    static void remove(QQmlEngineDebug *, QQmlDebugRootContextQuery *);
    static void remove(QQmlEngineDebug *, QQmlDebugObjectQuery *);
    static void remove(QQmlEngineDebug *, QQmlDebugExpressionQuery *);
    static void remove(QQmlEngineDebug *, QQmlDebugWatch *);

    QHash<int, QQmlDebugEnginesQuery *> enginesQuery;
    QHash<int, QQmlDebugRootContextQuery *> rootContextQuery;
    QHash<int, QQmlDebugObjectQuery *> objectQuery;
    QHash<int, QQmlDebugExpressionQuery *> expressionQuery;

    QHash<int, QQmlDebugWatch *> watched;
};

QQmlEngineDebugClient::QQmlEngineDebugClient(QQmlDebugConnection *client,
                                                             QQmlEngineDebugPrivate *p)
    : QQmlDebugClient(QLatin1String("QDeclarativeEngine"), client), priv(p)
{
}

void QQmlEngineDebugClient::stateChanged(State status)
{
    if (priv)
        priv->stateChanged(static_cast<QQmlEngineDebug::State>(status));
}

void QQmlEngineDebugClient::messageReceived(const QByteArray &data)
{
    if (priv)
        priv->message(data);
}

QQmlEngineDebugPrivate::QQmlEngineDebugPrivate(QQmlDebugConnection *c)
    : client(new QQmlEngineDebugClient(c, this)), nextId(0)
{
}

QQmlEngineDebugPrivate::~QQmlEngineDebugPrivate()
{
    if (client)
        client->priv = 0;
    delete client;

    QHash<int, QQmlDebugEnginesQuery*>::iterator enginesIter = enginesQuery.begin();
    for (; enginesIter != enginesQuery.end(); ++enginesIter) {
        enginesIter.value()->m_client = 0;
        if (enginesIter.value()->state() == QQmlDebugQuery::Waiting)
            enginesIter.value()->setState(QQmlDebugQuery::Error);
    }

    QHash<int, QQmlDebugRootContextQuery*>::iterator rootContextIter = rootContextQuery.begin();
    for (; rootContextIter != rootContextQuery.end(); ++rootContextIter) {
        rootContextIter.value()->m_client = 0;
        if (rootContextIter.value()->state() == QQmlDebugQuery::Waiting)
            rootContextIter.value()->setState(QQmlDebugQuery::Error);
    }

    QHash<int, QQmlDebugObjectQuery*>::iterator objectIter = objectQuery.begin();
    for (; objectIter != objectQuery.end(); ++objectIter) {
        objectIter.value()->m_client = 0;
        if (objectIter.value()->state() == QQmlDebugQuery::Waiting)
            objectIter.value()->setState(QQmlDebugQuery::Error);
    }

    QHash<int, QQmlDebugExpressionQuery*>::iterator exprIter = expressionQuery.begin();
    for (; exprIter != expressionQuery.end(); ++exprIter) {
        exprIter.value()->m_client = 0;
        if (exprIter.value()->state() == QQmlDebugQuery::Waiting)
            exprIter.value()->setState(QQmlDebugQuery::Error);
    }

    QHash<int, QQmlDebugWatch*>::iterator watchIter = watched.begin();
    for (; watchIter != watched.end(); ++watchIter) {
        watchIter.value()->m_client = 0;
        watchIter.value()->setState(QQmlDebugWatch::Dead);
    }
}

int QQmlEngineDebugPrivate::getId()
{
    return nextId++;
}

void QQmlEngineDebugPrivate::remove(QQmlEngineDebug *c, QQmlDebugEnginesQuery *q)
{
    if (c && q) {
        QQmlEngineDebugPrivate *p = (QQmlEngineDebugPrivate *)QObjectPrivate::get(c);
        p->enginesQuery.remove(q->m_queryId);
    }
}

void QQmlEngineDebugPrivate::remove(QQmlEngineDebug *c, 
                                            QQmlDebugRootContextQuery *q)
{
    if (c && q) {
        QQmlEngineDebugPrivate *p = (QQmlEngineDebugPrivate *)QObjectPrivate::get(c);
        p->rootContextQuery.remove(q->m_queryId);
    }
}

void QQmlEngineDebugPrivate::remove(QQmlEngineDebug *c, QQmlDebugObjectQuery *q)
{
    if (c && q) {
        QQmlEngineDebugPrivate *p = (QQmlEngineDebugPrivate *)QObjectPrivate::get(c);
        p->objectQuery.remove(q->m_queryId);
    }
}

void QQmlEngineDebugPrivate::remove(QQmlEngineDebug *c, QQmlDebugExpressionQuery *q)
{
    if (c && q) {
        QQmlEngineDebugPrivate *p = (QQmlEngineDebugPrivate *)QObjectPrivate::get(c);
        p->expressionQuery.remove(q->m_queryId);
    }
}

void QQmlEngineDebugPrivate::remove(QQmlEngineDebug *c, QQmlDebugWatch *w)
{
    if (c && w) {
        QQmlEngineDebugPrivate *p = (QQmlEngineDebugPrivate *)QObjectPrivate::get(c);
        p->watched.remove(w->m_queryId);
    }
}

void QQmlEngineDebugPrivate::decode(QDataStream &ds, QQmlDebugObjectReference &o,
                                            bool simple)
{
    QQmlEngineDebugService::QQmlObjectData data;
    ds >> data;
    o.m_debugId = data.objectId;
    o.m_class = data.objectType;
    o.m_idString = data.idString;
    o.m_name = data.objectName;
    o.m_source.m_url = data.url;
    o.m_source.m_lineNumber = data.lineNumber;
    o.m_source.m_columnNumber = data.columnNumber;
    o.m_contextDebugId = data.contextId;

    if (simple)
        return;

    int childCount;
    bool recur;
    ds >> childCount >> recur;

    for (int ii = 0; ii < childCount; ++ii) {
        o.m_children.append(QQmlDebugObjectReference());
        decode(ds, o.m_children.last(), !recur);
    }

    int propCount;
    ds >> propCount;

    for (int ii = 0; ii < propCount; ++ii) {
        QQmlEngineDebugService::QQmlObjectProperty data;
        ds >> data;
        QQmlDebugPropertyReference prop;
        prop.m_objectDebugId = o.m_debugId;
        prop.m_name = data.name;
        prop.m_binding = data.binding;
        prop.m_hasNotifySignal = data.hasNotifySignal;
        prop.m_valueTypeName = data.valueTypeName;
        switch (data.type) {
        case QQmlEngineDebugService::QQmlObjectProperty::Basic:
        case QQmlEngineDebugService::QQmlObjectProperty::List:
        case QQmlEngineDebugService::QQmlObjectProperty::SignalProperty:
        {
            prop.m_value = data.value;
            break;
        }
        case QQmlEngineDebugService::QQmlObjectProperty::Object:
        {
            QQmlDebugObjectReference obj;
            obj.m_debugId = prop.m_value.toInt();
            prop.m_value = QVariant::fromValue(obj);
            break;
        }
        case QQmlEngineDebugService::QQmlObjectProperty::Unknown:
            break;
        }
        o.m_properties << prop;
    }
}

void QQmlEngineDebugPrivate::decode(QDataStream &ds, QQmlDebugContextReference &c)
{
    ds >> c.m_name >> c.m_debugId;

    int contextCount;
    ds >> contextCount;

    for (int ii = 0; ii < contextCount; ++ii) {
        c.m_contexts.append(QQmlDebugContextReference());
        decode(ds, c.m_contexts.last());
    }

    int objectCount;
    ds >> objectCount;

    for (int ii = 0; ii < objectCount; ++ii) {
        QQmlDebugObjectReference obj;
        decode(ds, obj, true);

        obj.m_contextDebugId = c.m_debugId;
        c.m_objects << obj;
    }
}

void QQmlEngineDebugPrivate::stateChanged(QQmlEngineDebug::State status)
{
    emit q_func()->stateChanged(status);
}

void QQmlEngineDebugPrivate::message(const QByteArray &data)
{
    QDataStream ds(data);

    QByteArray type;
    ds >> type;

    //qDebug() << "QQmlEngineDebugPrivate::message()" << type;

    if (type == "LIST_ENGINES_R") {
        int queryId;
        ds >> queryId;

        QQmlDebugEnginesQuery *query = enginesQuery.value(queryId);
        if (!query)
            return;
        enginesQuery.remove(queryId);

        int count;
        ds >> count;

        for (int ii = 0; ii < count; ++ii) {
            QQmlDebugEngineReference ref;
            ds >> ref.m_name;
            ds >> ref.m_debugId;
            query->m_engines << ref;
        }

        query->m_client = 0;
        query->setState(QQmlDebugQuery::Completed);
    } else if (type == "LIST_OBJECTS_R") {
        int queryId;
        ds >> queryId;

        QQmlDebugRootContextQuery *query = rootContextQuery.value(queryId);
        if (!query)
            return;
        rootContextQuery.remove(queryId);

        if (!ds.atEnd())
            decode(ds, query->m_context);

        query->m_client = 0;
        query->setState(QQmlDebugQuery::Completed);
    } else if (type == "FETCH_OBJECT_R") {
        int queryId;
        ds >> queryId;

        QQmlDebugObjectQuery *query = objectQuery.value(queryId);
        if (!query)
            return;
        objectQuery.remove(queryId);

        if (!ds.atEnd())
            decode(ds, query->m_object, false);

        query->m_client = 0;
        query->setState(QQmlDebugQuery::Completed);
    } else if (type == "EVAL_EXPRESSION_R") {
        int queryId;
        QVariant result;
        ds >> queryId >> result;

        QQmlDebugExpressionQuery *query = expressionQuery.value(queryId);
        if (!query)
            return;
        expressionQuery.remove(queryId);

        query->m_result = result;
        query->m_client = 0;
        query->setState(QQmlDebugQuery::Completed);
    } else if (type == "WATCH_PROPERTY_R") {
        int queryId;
        bool ok;
        ds >> queryId >> ok;

        QQmlDebugWatch *watch = watched.value(queryId);
        if (!watch)
            return;

        watch->setState(ok ? QQmlDebugWatch::Active : QQmlDebugWatch::Inactive);
    } else if (type == "WATCH_OBJECT_R") {
        int queryId;
        bool ok;
        ds >> queryId >> ok;

        QQmlDebugWatch *watch = watched.value(queryId);
        if (!watch)
            return;

        watch->setState(ok ? QQmlDebugWatch::Active : QQmlDebugWatch::Inactive);
    } else if (type == "WATCH_EXPR_OBJECT_R") {
        int queryId;
        bool ok;
        ds >> queryId >> ok;

        QQmlDebugWatch *watch = watched.value(queryId);
        if (!watch)
            return;

        watch->setState(ok ? QQmlDebugWatch::Active : QQmlDebugWatch::Inactive);
    } else if (type == "UPDATE_WATCH") {
        int queryId;
        int debugId;
        QByteArray name;
        QVariant value;
        ds >> queryId >> debugId >> name >> value;

        QQmlDebugWatch *watch = watched.value(queryId, 0);
        if (!watch)
            return;
        emit watch->valueChanged(name, value);
    } else if (type == "OBJECT_CREATED") {
        emit q_func()->newObjects();
    }
}

QQmlEngineDebug::QQmlEngineDebug(QQmlDebugConnection *client, QObject *parent)
    : QObject(*(new QQmlEngineDebugPrivate(client)), parent)
{
}

QQmlEngineDebug::~QQmlEngineDebug()
{
}

QQmlEngineDebug::State QQmlEngineDebug::state() const
{
    Q_D(const QQmlEngineDebug);

    return static_cast<QQmlEngineDebug::State>(d->client->state());
}

QQmlDebugPropertyWatch *QQmlEngineDebug::addWatch(const QQmlDebugPropertyReference &property, QObject *parent)
{
    Q_D(QQmlEngineDebug);

    QQmlDebugPropertyWatch *watch = new QQmlDebugPropertyWatch(parent);
    if (d->client->state() == QQmlDebugClient::Enabled) {
        int queryId = d->getId();
        watch->m_queryId = queryId;
        watch->m_client = this;
        watch->m_objectDebugId = property.objectDebugId();
        watch->m_name = property.name();
        d->watched.insert(queryId, watch);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_PROPERTY") << queryId << property.objectDebugId() << property.name().toUtf8();
        d->client->sendMessage(message);
    } else {
        watch->m_state = QQmlDebugWatch::Dead;
    }

    return watch;
}

QQmlDebugWatch *QQmlEngineDebug::addWatch(const QQmlDebugContextReference &, const QString &, QObject *)
{
    qWarning("QQmlEngineDebug::addWatch(): Not implemented");
    return 0;
}

QQmlDebugObjectExpressionWatch *QQmlEngineDebug::addWatch(const QQmlDebugObjectReference &object, const QString &expr, QObject *parent)
{
    Q_D(QQmlEngineDebug);
    QQmlDebugObjectExpressionWatch *watch = new QQmlDebugObjectExpressionWatch(parent);
    if (d->client->state() == QQmlDebugClient::Enabled) {
        int queryId = d->getId();
        watch->m_queryId = queryId;
        watch->m_client = this;
        watch->m_objectDebugId = object.debugId();
        watch->m_expr = expr;
        d->watched.insert(queryId, watch);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_EXPR_OBJECT") << queryId << object.debugId() << expr;
        d->client->sendMessage(message);
    } else {
        watch->m_state = QQmlDebugWatch::Dead;
    }
    return watch;
}

QQmlDebugWatch *QQmlEngineDebug::addWatch(const QQmlDebugObjectReference &object, QObject *parent)
{
    Q_D(QQmlEngineDebug);

    QQmlDebugWatch *watch = new QQmlDebugWatch(parent);
    if (d->client->state() == QQmlDebugClient::Enabled) {
        int queryId = d->getId();
        watch->m_queryId = queryId;
        watch->m_client = this;
        watch->m_objectDebugId = object.debugId();
        d->watched.insert(queryId, watch);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_OBJECT") << queryId << object.debugId();
        d->client->sendMessage(message);
    } else {
        watch->m_state = QQmlDebugWatch::Dead;
    }

    return watch;
}

QQmlDebugWatch *QQmlEngineDebug::addWatch(const QQmlDebugFileReference &, QObject *)
{
    qWarning("QQmlEngineDebug::addWatch(): Not implemented");
    return 0;
}

void QQmlEngineDebug::removeWatch(QQmlDebugWatch *watch)
{
    Q_D(QQmlEngineDebug);

    if (!watch || !watch->m_client)
        return;

    watch->m_client = 0;
    watch->setState(QQmlDebugWatch::Inactive);
    
    d->watched.remove(watch->queryId());

    if (d->client && d->client->state() == QQmlDebugClient::Enabled) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("NO_WATCH") << watch->queryId();
        d->client->sendMessage(message);
    }
}

QQmlDebugEnginesQuery *QQmlEngineDebug::queryAvailableEngines(QObject *parent)
{
    Q_D(QQmlEngineDebug);

    QQmlDebugEnginesQuery *query = new QQmlDebugEnginesQuery(parent);
    if (d->client->state() == QQmlDebugClient::Enabled) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->enginesQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("LIST_ENGINES") << queryId;
        d->client->sendMessage(message);
    } else {
        query->m_state = QQmlDebugQuery::Error;
    }

    return query;
}

QQmlDebugRootContextQuery *QQmlEngineDebug::queryRootContexts(const QQmlDebugEngineReference &engine, QObject *parent)
{
    Q_D(QQmlEngineDebug);

    QQmlDebugRootContextQuery *query = new QQmlDebugRootContextQuery(parent);
    if (d->client->state() == QQmlDebugClient::Enabled && engine.debugId() != -1) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->rootContextQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("LIST_OBJECTS") << queryId << engine.debugId();
        d->client->sendMessage(message);
    } else {
        query->m_state = QQmlDebugQuery::Error;
    }

    return query;
}

QQmlDebugObjectQuery *QQmlEngineDebug::queryObject(const QQmlDebugObjectReference &object, QObject *parent)
{
    Q_D(QQmlEngineDebug);

    QQmlDebugObjectQuery *query = new QQmlDebugObjectQuery(parent);
    if (d->client->state() == QQmlDebugClient::Enabled && object.debugId() != -1) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->objectQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("FETCH_OBJECT") << queryId << object.debugId()
           << false << true;
        d->client->sendMessage(message);
    } else {
        query->m_state = QQmlDebugQuery::Error;
    }

    return query;
}

QQmlDebugObjectQuery *QQmlEngineDebug::queryObjectRecursive(const QQmlDebugObjectReference &object, QObject *parent)
{
    Q_D(QQmlEngineDebug);

    QQmlDebugObjectQuery *query = new QQmlDebugObjectQuery(parent);
    if (d->client->state() == QQmlDebugClient::Enabled && object.debugId() != -1) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->objectQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("FETCH_OBJECT") << queryId << object.debugId()
           << true << true;
        d->client->sendMessage(message);
    } else {
        query->m_state = QQmlDebugQuery::Error;
    }

    return query;
}

QQmlDebugExpressionQuery *QQmlEngineDebug::queryExpressionResult(int objectDebugId, const QString &expr, QObject *parent)
{
    Q_D(QQmlEngineDebug);

    QQmlDebugExpressionQuery *query = new QQmlDebugExpressionQuery(parent);
    if (d->client->state() == QQmlDebugClient::Enabled && objectDebugId != -1) {
        query->m_client = this;
        query->m_expr = expr;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->expressionQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("EVAL_EXPRESSION") << queryId << objectDebugId << expr;
        d->client->sendMessage(message);
    } else {
        query->m_state = QQmlDebugQuery::Error;
    }

    return query;
}

bool QQmlEngineDebug::setBindingForObject(int objectDebugId, const QString &propertyName,
                                                  const QVariant &bindingExpression,
                                                  bool isLiteralValue,
                                                  QString source, int line)
{
    Q_D(QQmlEngineDebug);

    if (d->client->state() == QQmlDebugClient::Enabled && objectDebugId != -1) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_BINDING") << objectDebugId << propertyName << bindingExpression << isLiteralValue << source << line;
        d->client->sendMessage(message);
        return true;
    } else {
        return false;
    }
}

bool QQmlEngineDebug::resetBindingForObject(int objectDebugId, const QString &propertyName)
{
    Q_D(QQmlEngineDebug);

    if (d->client->state() == QQmlDebugClient::Enabled && objectDebugId != -1) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("RESET_BINDING") << objectDebugId << propertyName;
        d->client->sendMessage(message);
        return true;
    } else {
        return false;
    }
}

bool QQmlEngineDebug::setMethodBody(int objectDebugId, const QString &methodName,
                                            const QString &methodBody)
{
    Q_D(QQmlEngineDebug);

    if (d->client->state() == QQmlDebugClient::Enabled && objectDebugId != -1) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_METHOD_BODY") << objectDebugId << methodName << methodBody;
        d->client->sendMessage(message);
        return true;
    } else {
        return false;
    }
}

QQmlDebugWatch::QQmlDebugWatch(QObject *parent)
    : QObject(parent), m_state(Waiting), m_queryId(-1), m_client(0), m_objectDebugId(-1)
{
}

QQmlDebugWatch::~QQmlDebugWatch()
{
    if (m_client && m_queryId != -1)
        QQmlEngineDebugPrivate::remove(m_client, this);
}

int QQmlDebugWatch::queryId() const
{
    return m_queryId;
}

int QQmlDebugWatch::objectDebugId() const
{
    return m_objectDebugId;
}

QQmlDebugWatch::State QQmlDebugWatch::state() const
{
    return m_state;
}

void QQmlDebugWatch::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(m_state);
}

QQmlDebugPropertyWatch::QQmlDebugPropertyWatch(QObject *parent)
    : QQmlDebugWatch(parent)
{
}

QString QQmlDebugPropertyWatch::name() const
{
    return m_name;
}


QQmlDebugObjectExpressionWatch::QQmlDebugObjectExpressionWatch(QObject *parent)
    : QQmlDebugWatch(parent)
{
}

QString QQmlDebugObjectExpressionWatch::expression() const
{
    return m_expr;
}


QQmlDebugQuery::QQmlDebugQuery(QObject *parent)
    : QObject(parent), m_state(Waiting)
{
}

QQmlDebugQuery::State QQmlDebugQuery::state() const
{
    return m_state;
}

bool QQmlDebugQuery::isWaiting() const
{
    return m_state == Waiting;
}

void QQmlDebugQuery::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(m_state);
}

QQmlDebugEnginesQuery::QQmlDebugEnginesQuery(QObject *parent)
    : QQmlDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QQmlDebugEnginesQuery::~QQmlDebugEnginesQuery()
{
    if (m_client && m_queryId != -1)
        QQmlEngineDebugPrivate::remove(m_client, this);
}

QList<QQmlDebugEngineReference> QQmlDebugEnginesQuery::engines() const
{
    return m_engines;
}

QQmlDebugRootContextQuery::QQmlDebugRootContextQuery(QObject *parent)
    : QQmlDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QQmlDebugRootContextQuery::~QQmlDebugRootContextQuery()
{
    if (m_client && m_queryId != -1)
        QQmlEngineDebugPrivate::remove(m_client, this);
}

QQmlDebugContextReference QQmlDebugRootContextQuery::rootContext() const
{
    return m_context;
}

QQmlDebugObjectQuery::QQmlDebugObjectQuery(QObject *parent)
    : QQmlDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QQmlDebugObjectQuery::~QQmlDebugObjectQuery()
{
    if (m_client && m_queryId != -1)
        QQmlEngineDebugPrivate::remove(m_client, this);
}

QQmlDebugObjectReference QQmlDebugObjectQuery::object() const
{
    return m_object;
}

QQmlDebugExpressionQuery::QQmlDebugExpressionQuery(QObject *parent)
    : QQmlDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QQmlDebugExpressionQuery::~QQmlDebugExpressionQuery()
{
    if (m_client && m_queryId != -1)
        QQmlEngineDebugPrivate::remove(m_client, this);
}

QVariant QQmlDebugExpressionQuery::expression() const
{
    return m_expr;
}

QVariant QQmlDebugExpressionQuery::result() const
{
    return m_result;
}

QQmlDebugEngineReference::QQmlDebugEngineReference()
    : m_debugId(-1)
{
}

QQmlDebugEngineReference::QQmlDebugEngineReference(int debugId)
    : m_debugId(debugId)
{
}

QQmlDebugEngineReference::QQmlDebugEngineReference(const QQmlDebugEngineReference &o)
    : m_debugId(o.m_debugId), m_name(o.m_name)
{
}

QQmlDebugEngineReference &
QQmlDebugEngineReference::operator=(const QQmlDebugEngineReference &o)
{
    m_debugId = o.m_debugId; m_name = o.m_name;
    return *this;
}

int QQmlDebugEngineReference::debugId() const
{
    return m_debugId;
}

QString QQmlDebugEngineReference::name() const
{
    return m_name;
}

QQmlDebugObjectReference::QQmlDebugObjectReference()
    : m_debugId(-1), m_contextDebugId(-1)
{
}

QQmlDebugObjectReference::QQmlDebugObjectReference(int debugId)
    : m_debugId(debugId), m_contextDebugId(-1)
{
}

QQmlDebugObjectReference::QQmlDebugObjectReference(const QQmlDebugObjectReference &o)
    : m_debugId(o.m_debugId), m_class(o.m_class), m_idString(o.m_idString),
      m_name(o.m_name), m_source(o.m_source), m_contextDebugId(o.m_contextDebugId),
      m_properties(o.m_properties), m_children(o.m_children)
{
}

QQmlDebugObjectReference &
QQmlDebugObjectReference::operator=(const QQmlDebugObjectReference &o)
{
    m_debugId = o.m_debugId; m_class = o.m_class; m_idString = o.m_idString;
    m_name = o.m_name; m_source = o.m_source; m_contextDebugId = o.m_contextDebugId;
    m_properties = o.m_properties; m_children = o.m_children;
    return *this;
}

int QQmlDebugObjectReference::debugId() const
{
    return m_debugId;
}

QString QQmlDebugObjectReference::className() const
{
    return m_class;
}

QString QQmlDebugObjectReference::idString() const
{
    return m_idString;
}

QString QQmlDebugObjectReference::name() const
{
    return m_name;
}

QQmlDebugFileReference QQmlDebugObjectReference::source() const
{
    return m_source;
}

int QQmlDebugObjectReference::contextDebugId() const
{
    return m_contextDebugId;
}

QList<QQmlDebugPropertyReference> QQmlDebugObjectReference::properties() const
{
    return m_properties;
}

QList<QQmlDebugObjectReference> QQmlDebugObjectReference::children() const
{
    return m_children;
}

QQmlDebugContextReference::QQmlDebugContextReference()
    : m_debugId(-1)
{
}

QQmlDebugContextReference::QQmlDebugContextReference(const QQmlDebugContextReference &o)
    : m_debugId(o.m_debugId), m_name(o.m_name), m_objects(o.m_objects), m_contexts(o.m_contexts)
{
}

QQmlDebugContextReference &QQmlDebugContextReference::operator=(const QQmlDebugContextReference &o)
{
    m_debugId = o.m_debugId; m_name = o.m_name; m_objects = o.m_objects;
    m_contexts = o.m_contexts;
    return *this;
}

int QQmlDebugContextReference::debugId() const
{
    return m_debugId;
}

QString QQmlDebugContextReference::name() const
{
    return m_name;
}

QList<QQmlDebugObjectReference> QQmlDebugContextReference::objects() const
{
    return m_objects;
}

QList<QQmlDebugContextReference> QQmlDebugContextReference::contexts() const
{
    return m_contexts;
}

QQmlDebugFileReference::QQmlDebugFileReference()
    : m_lineNumber(-1), m_columnNumber(-1)
{
}

QQmlDebugFileReference::QQmlDebugFileReference(const QQmlDebugFileReference &o)
    : m_url(o.m_url), m_lineNumber(o.m_lineNumber), m_columnNumber(o.m_columnNumber)
{
}

QQmlDebugFileReference &QQmlDebugFileReference::operator=(const QQmlDebugFileReference &o)
{
    m_url = o.m_url; m_lineNumber = o.m_lineNumber; m_columnNumber = o.m_columnNumber;
    return *this;
}

QUrl QQmlDebugFileReference::url() const
{
    return m_url;
}

void QQmlDebugFileReference::setUrl(const QUrl &u)
{
    m_url = u;
}

int QQmlDebugFileReference::lineNumber() const
{
    return m_lineNumber;
}

void QQmlDebugFileReference::setLineNumber(int l)
{
    m_lineNumber = l;
}

int QQmlDebugFileReference::columnNumber() const
{
    return m_columnNumber;
}

void QQmlDebugFileReference::setColumnNumber(int c)
{
    m_columnNumber = c;
}

QQmlDebugPropertyReference::QQmlDebugPropertyReference()
    : m_objectDebugId(-1), m_hasNotifySignal(false)
{
}

QQmlDebugPropertyReference::QQmlDebugPropertyReference(const QQmlDebugPropertyReference &o)
    : m_objectDebugId(o.m_objectDebugId), m_name(o.m_name), m_value(o.m_value),
      m_valueTypeName(o.m_valueTypeName), m_binding(o.m_binding),
      m_hasNotifySignal(o.m_hasNotifySignal)
{
}

QQmlDebugPropertyReference &QQmlDebugPropertyReference::operator=(const QQmlDebugPropertyReference &o)
{
    m_objectDebugId = o.m_objectDebugId; m_name = o.m_name; m_value = o.m_value;
    m_valueTypeName = o.m_valueTypeName; m_binding = o.m_binding;
    m_hasNotifySignal = o.m_hasNotifySignal;
    return *this;
}

int QQmlDebugPropertyReference::objectDebugId() const
{
    return m_objectDebugId;
}

QString QQmlDebugPropertyReference::name() const
{
    return m_name;
}

QString QQmlDebugPropertyReference::valueTypeName() const
{
    return m_valueTypeName;
}

QVariant QQmlDebugPropertyReference::value() const
{
    return m_value;
}

QString QQmlDebugPropertyReference::binding() const
{
    return m_binding;
}

bool QQmlDebugPropertyReference::hasNotifySignal() const
{
    return m_hasNotifySignal;
}

