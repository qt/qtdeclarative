/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlenginedebugclient_p_p.h"
#include <private/qqmldebugconnection_p.h>

QT_BEGIN_NAMESPACE

struct QQmlObjectData {
    QUrl url;
    qint32 lineNumber = -1;
    qint32 columnNumber = -1;
    QString idString;
    QString objectName;
    QString objectType;
    qint32 objectId = -1;
    qint32 contextId = -1;
    qint32 parentId = -1;
};

QPacket &operator>>(QPacket &ds, QQmlObjectData &data)
{
    ds >> data.url >> data.lineNumber >> data.columnNumber >> data.idString
       >> data.objectName >> data.objectType >> data.objectId >> data.contextId
       >> data.parentId;
    return ds;
}

struct QQmlObjectProperty {
    enum Type { Unknown, Basic, Object, List, SignalProperty };
    Type type = Unknown;
    QString name;
    QVariant value;
    QString valueTypeName;
    QString binding;
    bool hasNotifySignal = false;
};

QPacket &operator>>(QPacket &ds, QQmlObjectProperty &data)
{
    qint32 type;
    ds >> type >> data.name >> data.value >> data.valueTypeName
       >> data.binding >> data.hasNotifySignal;
    data.type = QQmlObjectProperty::Type(type);
    return ds;
}

QQmlEngineDebugClient::QQmlEngineDebugClient(QQmlDebugConnection *connection) :
    QQmlDebugClient(*new QQmlEngineDebugClientPrivate(connection))
{
}

QQmlEngineDebugClientPrivate::QQmlEngineDebugClientPrivate(QQmlDebugConnection *connection) :
    QQmlDebugClientPrivate (QLatin1String("QmlDebugger"), connection)
{
}


qint32 QQmlEngineDebugClient::addWatch(
        const QQmlEngineDebugPropertyReference &property, bool *success)
{
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("WATCH_PROPERTY") << id << property.objectDebugId
           << property.name.toUtf8();
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::addWatch(
        const QQmlEngineDebugContextReference &, const QString &, bool *success)
{
    *success = false;
    qWarning("QQmlEngineDebugClient::addWatch(): Not implemented");
    return -1;
}

qint32 QQmlEngineDebugClient::addWatch(
        const QQmlEngineDebugObjectReference &object, const QString &expr,
        bool *success)
{
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("WATCH_EXPR_OBJECT") << id << object.debugId << expr;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::addWatch(
        const QQmlEngineDebugObjectReference &object, bool *success)
{
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("WATCH_OBJECT") << id << object.debugId;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::addWatch(
        const QQmlEngineDebugFileReference &,  bool *success)
{
    *success = false;
    qWarning("QQmlEngineDebugClient::addWatch(): Not implemented");
    return -1;
}

void QQmlEngineDebugClient::removeWatch(qint32 id, bool *success)
{
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("NO_WATCH") << id;
        sendMessage(ds.data());
        *success = true;
    }
}

qint32 QQmlEngineDebugClient::queryAvailableEngines(bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->engines.clear();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("LIST_ENGINES") << id;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::queryRootContexts(
        const QQmlEngineDebugEngineReference &engine, bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->rootContext = QQmlEngineDebugContextReference();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled && engine.debugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("LIST_OBJECTS") << id << engine.debugId;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::queryObject(
        const QQmlEngineDebugObjectReference &object, bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->object = QQmlEngineDebugObjectReference();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled && object.debugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("FETCH_OBJECT") << id << object.debugId << false << true;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::queryObjectsForLocation(
        const QString &file, qint32 lineNumber, qint32 columnNumber, bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->objects.clear();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("FETCH_OBJECTS_FOR_LOCATION") << id << file << lineNumber
           << columnNumber << false << true;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::queryObjectRecursive(
        const QQmlEngineDebugObjectReference &object, bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->object = QQmlEngineDebugObjectReference();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled && object.debugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("FETCH_OBJECT") << id << object.debugId << true << true;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::queryObjectsForLocationRecursive(const QString &file,
        qint32 lineNumber, qint32 columnNumber, bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->objects.clear();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("FETCH_OBJECTS_FOR_LOCATION") << id << file << lineNumber
           << columnNumber << true << true;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::queryExpressionResult(
        qint32 objectDebugId, const QString &expr, bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->exprResult = QVariant();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("EVAL_EXPRESSION") << id << objectDebugId << expr
           << engines()[0].debugId;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::queryExpressionResultBC(
        qint32 objectDebugId, const QString &expr, bool *success)
{
    Q_D(QQmlEngineDebugClient);
    d->exprResult = QVariant();
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("EVAL_EXPRESSION") << id << objectDebugId << expr;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::setBindingForObject(
        qint32 objectDebugId,
        const QString &propertyName,
        const QVariant &bindingExpression,
        bool isLiteralValue,
        const QString &source, qint32 line,
        bool *success)
{
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("SET_BINDING") << id << objectDebugId << propertyName
           << bindingExpression << isLiteralValue << source << line;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::resetBindingForObject(
        qint32 objectDebugId,
        const QString &propertyName,
        bool *success)
{
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("RESET_BINDING") << id << objectDebugId << propertyName;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

qint32 QQmlEngineDebugClient::setMethodBody(
        qint32 objectDebugId, const QString &methodName,
        const QString &methodBody, bool *success)
{
    qint32 id = -1;
    *success = false;
    if (state() == QQmlDebugClient::Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("SET_METHOD_BODY") << id << objectDebugId
           << methodName << methodBody;
        sendMessage(ds.data());
        *success = true;
    }
    return id;
}

void QQmlEngineDebugClient::decode(QPacket &ds,
                                   QQmlEngineDebugObjectReference &o,
                                   bool simple)
{
    QQmlObjectData data;
    ds >> data;
    o.debugId = data.objectId;
    o.className = data.objectType;
    o.idString = data.idString;
    o.name = data.objectName;
    o.source.url = data.url;
    o.source.lineNumber = data.lineNumber;
    o.source.columnNumber = data.columnNumber;
    o.contextDebugId = data.contextId;

    if (simple)
        return;

    qint32 childCount;
    bool recur;
    ds >> childCount >> recur;

    for (qint32 ii = 0; ii < childCount; ++ii) {
        o.children.append(QQmlEngineDebugObjectReference());
        decode(ds, o.children.last(), !recur);
    }

    qint32 propCount;
    ds >> propCount;

    for (qint32 ii = 0; ii < propCount; ++ii) {
        QQmlObjectProperty data;
        ds >> data;
        QQmlEngineDebugPropertyReference prop;
        prop.objectDebugId = o.debugId;
        prop.name = data.name;
        prop.binding = data.binding;
        prop.hasNotifySignal = data.hasNotifySignal;
        prop.valueTypeName = data.valueTypeName;
        switch (data.type) {
        case QQmlObjectProperty::Basic:
        case QQmlObjectProperty::List:
        case QQmlObjectProperty::SignalProperty:
        {
            prop.value = data.value;
            break;
        }
        case QQmlObjectProperty::Object:
        {
            QQmlEngineDebugObjectReference obj;
            obj.name = data.value.toString();
            obj.className = prop.valueTypeName;
            prop.value = QVariant::fromValue(obj);
            break;
        }
        case QQmlObjectProperty::Unknown:
            break;
        }
        o.properties << prop;
    }
}

void QQmlEngineDebugClient::decode(QPacket &ds,
                                   QList<QQmlEngineDebugObjectReference> &o,
                                   bool simple)
{
    qint32 count;
    ds >> count;
    for (qint32 i = 0; i < count; i++) {
        QQmlEngineDebugObjectReference obj;
        decode(ds, obj, simple);
        o << obj;
    }
}

QList<QQmlEngineDebugEngineReference> QQmlEngineDebugClient::engines() const
{
    Q_D(const QQmlEngineDebugClient);
    return d->engines;
}

QQmlEngineDebugContextReference QQmlEngineDebugClient::rootContext() const
{
    Q_D(const QQmlEngineDebugClient);
    return d->rootContext;
}

QQmlEngineDebugObjectReference QQmlEngineDebugClient::object() const
{
    Q_D(const QQmlEngineDebugClient);
    return d->object;
}

QList<QQmlEngineDebugObjectReference> QQmlEngineDebugClient::objects() const
{
    Q_D(const QQmlEngineDebugClient);
    return d->objects;
}

QVariant QQmlEngineDebugClient::resultExpr() const
{
    Q_D(const QQmlEngineDebugClient);
    return d->exprResult;
}

bool QQmlEngineDebugClient::valid() const
{
    Q_D(const QQmlEngineDebugClient);
    return d->valid;
}

void QQmlEngineDebugClient::decode(QPacket &ds,
                                   QQmlEngineDebugContextReference &c)
{
    ds >> c.name >> c.debugId;

    qint32 contextCount;
    ds >> contextCount;

    for (qint32 ii = 0; ii < contextCount; ++ii) {
        c.contexts.append(QQmlEngineDebugContextReference());
        decode(ds, c.contexts.last());
    }

    qint32 objectCount;
    ds >> objectCount;

    for (qint32 ii = 0; ii < objectCount; ++ii) {
        QQmlEngineDebugObjectReference obj;
        decode(ds, obj, true);

        obj.contextDebugId = c.debugId;
        c.objects << obj;
    }
}

void QQmlEngineDebugClient::messageReceived(const QByteArray &data)
{
    Q_D(QQmlEngineDebugClient);
    d->valid = false;
    QPacket ds(connection()->currentDataStreamVersion(), data);

    qint32 queryId;
    QByteArray type;
    ds >> type >> queryId;

    //qDebug() << "QQmlEngineDebugPrivate::message()" << type;

    if (type == "LIST_ENGINES_R") {
        qint32 count;
        ds >> count;

        d->engines.clear();
        for (qint32 ii = 0; ii < count; ++ii) {
            QQmlEngineDebugEngineReference eng;
            ds >> eng.name;
            ds >> eng.debugId;
            d->engines << eng;
        }
    } else if (type == "LIST_OBJECTS_R") {
        if (!ds.atEnd())
            decode(ds, d->rootContext);

    } else if (type == "FETCH_OBJECT_R") {
        if (!ds.atEnd())
            decode(ds, d->object, false);

    } else if (type == "FETCH_OBJECTS_FOR_LOCATION_R") {
        if (!ds.atEnd())
            decode(ds, d->objects, false);

    } else if (type == "EVAL_EXPRESSION_R") {;
        ds >> d->exprResult;

    } else if (type == "WATCH_PROPERTY_R") {
        ds >> d->valid;

    } else if (type == "WATCH_OBJECT_R") {
        ds >> d->valid;

    } else if (type == "WATCH_EXPR_OBJECT_R") {
        ds >> d->valid;

    } else if (type == "UPDATE_WATCH") {
        qint32 debugId;
        QByteArray name;
        QVariant value;
        ds >> debugId >> name >> value;
        emit valueChanged(name, value);
        return;

    } else if (type == "OBJECT_CREATED") {
        qint32 engineId;
        qint32 objectId;
        qint32 parentId;
        ds >> engineId >> objectId >> parentId;
        emit newObject(objectId);
        return;
    } else if (type == "SET_BINDING_R") {
        ds >> d->valid;
    } else if (type == "RESET_BINDING_R") {
        ds >> d->valid;
    } else if (type == "SET_METHOD_BODY_R") {
        ds >> d->valid;
    } else if (type == "NO_WATCH_R") {
        ds >> d->valid;
    }
    emit result();
}


qint32 QQmlEngineDebugClient::getId()
{
    Q_D(QQmlEngineDebugClient);
    return d->nextId++;
}

QT_END_NAMESPACE
