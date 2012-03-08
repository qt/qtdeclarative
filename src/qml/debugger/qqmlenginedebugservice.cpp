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

#include "qqmlenginedebugservice_p.h"

#include "qqmldebugstatesdelegate_p.h"
#include <private/qqmlboundsignal_p.h>
#include <qqmlengine.h>
#include <private/qqmlmetatype_p.h>
#include <qqmlproperty.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlcontext_p.h>
#include <private/qqmlwatcher_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlexpression_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QQmlEngineDebugService, qmlEngineDebugService)

QQmlEngineDebugService *QQmlEngineDebugService::instance()
{
    return qmlEngineDebugService();
}

QQmlEngineDebugService::QQmlEngineDebugService(QObject *parent)
    : QQmlDebugService(QStringLiteral("QDeclarativeEngine"), 1, parent),
      m_watch(new QQmlWatcher(this)),
      m_statesDelegate(0)
{
    QObject::connect(m_watch, SIGNAL(propertyChanged(int,int,QMetaProperty,QVariant)),
                     this, SLOT(propertyChanged(int,int,QMetaProperty,QVariant)));

    registerService();
}

QQmlEngineDebugService::~QQmlEngineDebugService()
{
    delete m_statesDelegate;
}

QDataStream &operator<<(QDataStream &ds, 
                        const QQmlEngineDebugService::QQmlObjectData &data)
{
    ds << data.url << data.lineNumber << data.columnNumber << data.idString
       << data.objectName << data.objectType << data.objectId << data.contextId;
    return ds;
}

QDataStream &operator>>(QDataStream &ds, 
                        QQmlEngineDebugService::QQmlObjectData &data)
{
    ds >> data.url >> data.lineNumber >> data.columnNumber >> data.idString
       >> data.objectName >> data.objectType >> data.objectId >> data.contextId;
    return ds;
}

QDataStream &operator<<(QDataStream &ds, 
                        const QQmlEngineDebugService::QQmlObjectProperty &data)
{
    ds << (int)data.type << data.name << data.value << data.valueTypeName
       << data.binding << data.hasNotifySignal;
    return ds;
}

QDataStream &operator>>(QDataStream &ds,  
                        QQmlEngineDebugService::QQmlObjectProperty &data)
{
    int type;
    ds >> type >> data.name >> data.value >> data.valueTypeName
       >> data.binding >> data.hasNotifySignal;
    data.type = (QQmlEngineDebugService::QQmlObjectProperty::Type)type;
    return ds;
}

static inline bool isSignalPropertyName(const QString &signalName)
{
    // see QmlCompiler::isSignalPropertyName
    return signalName.length() >= 3 && signalName.startsWith(QLatin1String("on")) &&
            signalName.at(2).isLetter() && signalName.at(2).isUpper();
}

static bool hasValidSignal(QObject *object, const QString &propertyName)
{
    if (!isSignalPropertyName(propertyName))
        return false;

    QString signalName = propertyName.mid(2);
    signalName[0] = signalName.at(0).toLower();

    int sigIdx = QQmlPropertyPrivate::findSignalByName(object->metaObject(), signalName.toLatin1()).methodIndex();

    if (sigIdx == -1)
        return false;

    return true;
}

QQmlEngineDebugService::QQmlObjectProperty
QQmlEngineDebugService::propertyData(QObject *obj, int propIdx)
{
    QQmlObjectProperty rv;

    QMetaProperty prop = obj->metaObject()->property(propIdx);

    rv.type = QQmlObjectProperty::Unknown;
    rv.valueTypeName = QString::fromUtf8(prop.typeName());
    rv.name = QString::fromUtf8(prop.name());
    rv.hasNotifySignal = prop.hasNotifySignal();
    QQmlAbstractBinding *binding =
            QQmlPropertyPrivate::binding(QQmlProperty(obj, rv.name));
    if (binding)
        rv.binding = binding->expression();

    if (QQmlValueTypeFactory::isValueType(prop.userType())) {
        rv.type = QQmlObjectProperty::Basic;
    } else if (QQmlMetaType::isQObject(prop.userType()))  {
        rv.type = QQmlObjectProperty::Object;
    } else if (QQmlMetaType::isList(prop.userType())) {
        rv.type = QQmlObjectProperty::List;
    }

    QVariant value;
    if (rv.type != QQmlObjectProperty::Unknown && prop.userType() != 0) {
        value = prop.read(obj);
    }
    rv.value = valueContents(value);

    return rv;
}

QVariant QQmlEngineDebugService::valueContents(const QVariant &value) const
{
    int userType = value.userType();

    //QObject * is not streamable.
    //Convert all such instances to a String value

    if (value.type() == QVariant::List) {
        QVariantList contents;
        QVariantList list = value.toList();
        int count = list.size();
        for (int i = 0; i < count; i++)
            contents << valueContents(list.at(i));
        return contents;
    }

    if (value.type() == QVariant::Map) {
        QVariantMap contents;
        QMapIterator<QString, QVariant> i(value.toMap());
         while (i.hasNext()) {
             i.next();
             contents.insert(i.key(), valueContents(i.value()));
         }
        return contents;
    }

    if (QQmlValueTypeFactory::isValueType(userType))
        return value;

    if (QQmlMetaType::isQObject(userType)) {
        QObject *o = QQmlMetaType::toQObject(value);
        if (o) {
            QString name = o->objectName();
            if (name.isEmpty())
                name = QStringLiteral("<unnamed object>");
            return name;
        }
    }

    return QStringLiteral("<unknown value>");
}

void QQmlEngineDebugService::buildObjectDump(QDataStream &message, 
                                                     QObject *object, bool recur, bool dumpProperties)
{
    message << objectData(object);

    QObjectList children = object->children();
    
    int childrenCount = children.count();
    for (int ii = 0; ii < children.count(); ++ii) {
        if (qobject_cast<QQmlContext*>(children[ii]) || QQmlBoundSignal::cast(children[ii]))
            --childrenCount;
    }

    message << childrenCount << recur;

    QList<QQmlObjectProperty> fakeProperties;

    for (int ii = 0; ii < children.count(); ++ii) {
        QObject *child = children.at(ii);
        if (qobject_cast<QQmlContext*>(child))
            continue;
        QQmlBoundSignal *signal = QQmlBoundSignal::cast(child);
        if (signal) {
            if (!dumpProperties)
                continue;
            QQmlObjectProperty prop;
            prop.type = QQmlObjectProperty::SignalProperty;
            prop.hasNotifySignal = false;
            QQmlExpression *expr = signal->expression();
            if (expr) {
                prop.value = expr->expression();
                QObject *scope = expr->scopeObject();
                if (scope) {
                    QString sig = QString::fromLatin1(scope->metaObject()->method(signal->index()).signature());
                    int lparen = sig.indexOf(QLatin1Char('('));
                    if (lparen >= 0) {
                        QString methodName = sig.mid(0, lparen);
                        prop.name = QLatin1String("on") + methodName[0].toUpper()
                                + methodName.mid(1);
                    }
                }
            }
            fakeProperties << prop;
        } else {
            if (recur)
                buildObjectDump(message, child, recur, dumpProperties);
            else
                message << objectData(child);
        }
    }

    if (!dumpProperties) {
        message << 0;
        return;
    }

    QList<int> propertyIndexes;
    for (int ii = 0; ii < object->metaObject()->propertyCount(); ++ii) {
        if (object->metaObject()->property(ii).isScriptable())
            propertyIndexes << ii;
    }

    message << propertyIndexes.size() + fakeProperties.count();

    for (int ii = 0; ii < propertyIndexes.size(); ++ii)
        message << propertyData(object, propertyIndexes.at(ii));

    for (int ii = 0; ii < fakeProperties.count(); ++ii)
        message << fakeProperties[ii];
}

void QQmlEngineDebugService::prepareDeferredObjects(QObject *obj)
{
    qmlExecuteDeferred(obj);

    QObjectList children = obj->children();
    for (int ii = 0; ii < children.count(); ++ii) {
        QObject *child = children.at(ii);
        prepareDeferredObjects(child);
    }

}

void QQmlEngineDebugService::buildObjectList(QDataStream &message, QQmlContext *ctxt)
{
    QQmlContextData *p = QQmlContextData::get(ctxt);

    QString ctxtName = ctxt->objectName();
    int ctxtId = QQmlDebugService::idForObject(ctxt);

    message << ctxtName << ctxtId;

    int count = 0;

    QQmlContextData *child = p->childContexts;
    while (child) {
        ++count;
        child = child->nextChild;
    }

    message << count;

    child = p->childContexts;
    while (child) {
        buildObjectList(message, child->asQQmlContext());
        child = child->nextChild;
    }

    // Clean deleted objects
    QQmlContextPrivate *ctxtPriv = QQmlContextPrivate::get(ctxt);
    for (int ii = 0; ii < ctxtPriv->instances.count(); ++ii) {
        if (!ctxtPriv->instances.at(ii)) {
            ctxtPriv->instances.removeAt(ii);
            --ii;
        }
    }

    message << ctxtPriv->instances.count();
    for (int ii = 0; ii < ctxtPriv->instances.count(); ++ii) {
        message << objectData(ctxtPriv->instances.at(ii));
    }
}

void QQmlEngineDebugService::buildStatesList(QQmlContext *ctxt, bool cleanList)
{
    if (m_statesDelegate)
        m_statesDelegate->buildStatesList(ctxt, cleanList);
}

QQmlEngineDebugService::QQmlObjectData
QQmlEngineDebugService::objectData(QObject *object)
{
    QQmlData *ddata = QQmlData::get(object);
    QQmlObjectData rv;
    if (ddata && ddata->outerContext) {
        rv.url = ddata->outerContext->url;
        rv.lineNumber = ddata->lineNumber;
        rv.columnNumber = ddata->columnNumber;
    } else {
        rv.lineNumber = -1;
        rv.columnNumber = -1;
    }

    QQmlContext *context = qmlContext(object);
    if (context) {
        QQmlContextData *cdata = QQmlContextData::get(context);
        if (cdata)
            rv.idString = cdata->findObjectId(object);
    }

    rv.objectName = object->objectName();
    rv.objectId = QQmlDebugService::idForObject(object);
    rv.contextId = QQmlDebugService::idForObject(qmlContext(object));

    QQmlType *type = QQmlMetaType::qmlType(object->metaObject());
    if (type) {
        QString typeName = type->qmlTypeName();
        int lastSlash = typeName.lastIndexOf(QLatin1Char('/'));
        rv.objectType = lastSlash < 0 ? typeName : typeName.mid(lastSlash+1);
    } else {
        rv.objectType = QString::fromUtf8(object->metaObject()->className());
        int marker = rv.objectType.indexOf(QLatin1String("_QMLTYPE_"));
        if (marker != -1)
            rv.objectType = rv.objectType.left(marker);
    }

    return rv;
}

void QQmlEngineDebugService::messageReceived(const QByteArray &message)
{
    QMetaObject::invokeMethod(this, "processMessage", Qt::QueuedConnection, Q_ARG(QByteArray, message));
}

void QQmlEngineDebugService::processMessage(const QByteArray &message)
{
    QDataStream ds(message);

    QByteArray type;
    ds >> type;

    if (type == "LIST_ENGINES") {
        int queryId;
        ds >> queryId;

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("LIST_ENGINES_R");
        rs << queryId << m_engines.count();

        for (int ii = 0; ii < m_engines.count(); ++ii) {
            QQmlEngine *engine = m_engines.at(ii);

            QString engineName = engine->objectName();
            int engineId = QQmlDebugService::idForObject(engine);

            rs << engineName << engineId;
        }

        sendMessage(reply);
    } else if (type == "LIST_OBJECTS") {
        int queryId;
        int engineId = -1;
        ds >> queryId >> engineId;

        QQmlEngine *engine =
                qobject_cast<QQmlEngine *>(QQmlDebugService::objectForId(engineId));

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("LIST_OBJECTS_R") << queryId;

        if (engine) {
            buildObjectList(rs, engine->rootContext());
            buildStatesList(engine->rootContext(), true);
        }

        sendMessage(reply);
    } else if (type == "FETCH_OBJECT") {
        int queryId;
        int objectId;
        bool recurse;
        bool dumpProperties = true;

        ds >> queryId >> objectId >> recurse >> dumpProperties;

        QObject *object = QQmlDebugService::objectForId(objectId);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("FETCH_OBJECT_R") << queryId;

        if (object) {
            if (recurse)
                prepareDeferredObjects(object);
            buildObjectDump(rs, object, recurse, dumpProperties);
        }

        sendMessage(reply);
    } else if (type == "WATCH_OBJECT") {
        int queryId;
        int objectId;

        ds >> queryId >> objectId;
        bool ok = m_watch->addWatch(queryId, objectId);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_OBJECT_R") << queryId << ok;

        sendMessage(reply);
    } else if (type == "WATCH_PROPERTY") {
        int queryId;
        int objectId;
        QByteArray property;

        ds >> queryId >> objectId >> property;
        bool ok = m_watch->addWatch(queryId, objectId, property);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_PROPERTY_R") << queryId << ok;

        sendMessage(reply);
    } else if (type == "WATCH_EXPR_OBJECT") {
        int queryId;
        int debugId;
        QString expr;

        ds >> queryId >> debugId >> expr;
        bool ok = m_watch->addWatch(queryId, debugId, expr);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_EXPR_OBJECT_R") << queryId << ok;
        sendMessage(reply);
    } else if (type == "NO_WATCH") {
        int queryId;

        ds >> queryId;
        m_watch->removeWatch(queryId);
    } else if (type == "EVAL_EXPRESSION") {
        int queryId;
        int objectId;
        QString expr;

        ds >> queryId >> objectId >> expr;

        QObject *object = QQmlDebugService::objectForId(objectId);
        QQmlContext *context = qmlContext(object);
        QVariant result;
        if (object && context) {
            QQmlExpression exprObj(context, object, expr);
            bool undefined = false;
            QVariant value = exprObj.evaluate(&undefined);
            if (undefined)
                result = QString(QStringLiteral("<undefined>"));
            else
                result = valueContents(value);
        } else {
            result = QString(QStringLiteral("<unknown context>"));
        }

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("EVAL_EXPRESSION_R") << queryId << result;

        sendMessage(reply);
    } else if (type == "SET_BINDING") {
        int objectId;
        QString propertyName;
        QVariant expr;
        bool isLiteralValue;
        QString filename;
        int line;
        ds >> objectId >> propertyName >> expr >> isLiteralValue;
        if (!ds.atEnd()) { // backward compatibility from 2.1, 2.2
            ds >> filename >> line;
        }
        setBinding(objectId, propertyName, expr, isLiteralValue, filename, line);
    } else if (type == "RESET_BINDING") {
        int objectId;
        QString propertyName;
        ds >> objectId >> propertyName;
        resetBinding(objectId, propertyName);
    } else if (type == "SET_METHOD_BODY") {
        int objectId;
        QString methodName;
        QString methodBody;
        ds >> objectId >> methodName >> methodBody;
        setMethodBody(objectId, methodName, methodBody);
    }
}

void QQmlEngineDebugService::setBinding(int objectId,
                                                const QString &propertyName,
                                                const QVariant &expression,
                                                bool isLiteralValue,
                                                QString filename,
                                                int line,
                                                int column)
{
    QObject *object = objectForId(objectId);
    QQmlContext *context = qmlContext(object);

    if (object && context) {
        QQmlProperty property(object, propertyName, context);
        if (property.isValid()) {

            bool inBaseState = true;
            if (m_statesDelegate) {
                m_statesDelegate->updateBinding(context, property, expression, isLiteralValue,
                                                filename, line, column, &inBaseState);
            }

            if (inBaseState) {
                if (isLiteralValue) {
                    property.write(expression);
                } else if (hasValidSignal(object, propertyName)) {
                    QQmlExpression *qmlExpression = new QQmlExpression(context, object, expression.toString());
                    QQmlPropertyPrivate::setSignalExpression(property, qmlExpression);
                    qmlExpression->setSourceLocation(filename, line, column);
                } else if (property.isProperty()) {
                    QQmlBinding *binding = new QQmlBinding(expression.toString(), object, context);
                    binding->setTarget(property);
                    binding->setSourceLocation(filename, line, column);
                    binding->setNotifyOnValueChanged(true);
                    QQmlAbstractBinding *oldBinding = QQmlPropertyPrivate::setBinding(property, binding);
                    if (oldBinding)
                        oldBinding->destroy();
                    binding->update();
                } else {
                    qWarning() << "QQmlEngineDebugService::setBinding: unable to set property" << propertyName << "on object" << object;
                }
            }

        } else {
            // not a valid property
            bool ok = false;
            if (m_statesDelegate)
                ok = m_statesDelegate->setBindingForInvalidProperty(object, propertyName, expression, isLiteralValue);
            if (!ok)
                qWarning() << "QQmlEngineDebugService::setBinding: unable to set property" << propertyName << "on object" << object;
        }
    }
}

void QQmlEngineDebugService::resetBinding(int objectId, const QString &propertyName)
{
    QObject *object = objectForId(objectId);
    QQmlContext *context = qmlContext(object);

    if (object && context) {
        if (object->property(propertyName.toLatin1()).isValid()) {
            QQmlProperty property(object, propertyName);
            QQmlAbstractBinding *oldBinding = QQmlPropertyPrivate::binding(property);
            if (oldBinding) {
                QQmlAbstractBinding *oldBinding = QQmlPropertyPrivate::setBinding(property, 0);
                if (oldBinding)
                    oldBinding->destroy();
            }
            if (property.isResettable()) {
                // Note: this will reset the property in any case, without regard to states
                // Right now almost no QQuickItem has reset methods for its properties (with the
                // notable exception of QQuickAnchors), so this is not a big issue
                // later on, setBinding does take states into account
                property.reset();
            } else {
                // overwrite with default value
                if (QQmlType *objType = QQmlMetaType::qmlType(object->metaObject())) {
                    if (QObject *emptyObject = objType->create()) {
                        if (emptyObject->property(propertyName.toLatin1()).isValid()) {
                            QVariant defaultValue = QQmlProperty(emptyObject, propertyName).read();
                            if (defaultValue.isValid()) {
                                setBinding(objectId, propertyName, defaultValue, true);
                            }
                        }
                        delete emptyObject;
                    }
                }
            }
        } else if (hasValidSignal(object, propertyName)) {
            QQmlProperty property(object, propertyName, context);
            QQmlPropertyPrivate::setSignalExpression(property, 0);
        } else {
            if (m_statesDelegate)
                m_statesDelegate->resetBindingForInvalidProperty(object, propertyName);
        }
    }
}

void QQmlEngineDebugService::setMethodBody(int objectId, const QString &method, const QString &body)
{
    QObject *object = objectForId(objectId);
    QQmlContext *context = qmlContext(object);
    if (!object || !context || !context->engine())
        return;
    QQmlContextData *contextData = QQmlContextData::get(context);
    if (!contextData)
        return;

    QQmlPropertyData dummy;
    QQmlPropertyData *prop =
            QQmlPropertyCache::property(context->engine(), object, method, dummy);

    if (!prop || !prop->isVMEFunction())
        return;

    QMetaMethod metaMethod = object->metaObject()->method(prop->coreIndex);
    QList<QByteArray> paramNames = metaMethod.parameterNames();

    QString paramStr;
    for (int ii = 0; ii < paramNames.count(); ++ii) {
        if (ii != 0) paramStr.append(QLatin1Char(','));
        paramStr.append(QString::fromUtf8(paramNames.at(ii)));
    }

    QString jsfunction = QLatin1String("(function ") + method + QLatin1Char('(') + paramStr +
            QLatin1String(") {");
    jsfunction += body;
    jsfunction += QLatin1String("\n})");

    QQmlVMEMetaObject *vmeMetaObject =
            static_cast<QQmlVMEMetaObject*>(QObjectPrivate::get(object)->metaObject);
    Q_ASSERT(vmeMetaObject); // the fact we found the property above should guarentee this

    int lineNumber = vmeMetaObject->vmeMethodLineNumber(prop->coreIndex);
    vmeMetaObject->setVmeMethod(prop->coreIndex, QQmlExpressionPrivate::evalFunction(contextData, object, jsfunction, contextData->url.toString(), lineNumber));
}

void QQmlEngineDebugService::propertyChanged(int id, int objectId, const QMetaProperty &property, const QVariant &value)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);

    rs << QByteArray("UPDATE_WATCH") << id << objectId << QByteArray(property.name()) << valueContents(value);

    sendMessage(reply);
}

void QQmlEngineDebugService::addEngine(QQmlEngine *engine)
{
    Q_ASSERT(engine);
    Q_ASSERT(!m_engines.contains(engine));

    m_engines.append(engine);
}

void QQmlEngineDebugService::remEngine(QQmlEngine *engine)
{
    Q_ASSERT(engine);
    Q_ASSERT(m_engines.contains(engine));

    m_engines.removeAll(engine);
}

void QQmlEngineDebugService::objectCreated(QQmlEngine *engine, QObject *object)
{
    Q_ASSERT(engine);
    Q_ASSERT(m_engines.contains(engine));

    int engineId = QQmlDebugService::idForObject(engine);
    int objectId = QQmlDebugService::idForObject(object);

    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);

    rs << QByteArray("OBJECT_CREATED") << engineId << objectId;
    sendMessage(reply);
}

void QQmlEngineDebugService::setStatesDelegate(QQmlDebugStatesDelegate *delegate)
{
    m_statesDelegate = delegate;
}

QT_END_NAMESPACE
