// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlwatcher.h"

#include "qqmlexpression.h"
#include "qqmlcontext.h"
#include "qqml.h"

#include <private/qqmldebugservice_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlvaluetype_p.h>

#include <QtCore/qmetaobject.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE


class QQmlWatchProxy : public QObject
{
    Q_OBJECT
public:
    QQmlWatchProxy(int id,
                  QObject *object,
                  int debugId,
                  const QMetaProperty &prop,
                  QQmlWatcher *parent = nullptr);

    QQmlWatchProxy(int id,
                  QQmlExpression *exp,
                  int debugId,
                  QQmlWatcher *parent = nullptr);

public slots:
    void notifyValueChanged(); // Needs to be a slot because of QQmlPropertyPrivate::connect()

private:
    friend class QQmlWatcher;
    int m_id;
    QQmlWatcher *m_watch;
    QObject *m_object;
    int m_debugId;
    QMetaProperty m_property;

    QQmlExpression *m_expr;
};

QQmlWatchProxy::QQmlWatchProxy(int id, QQmlExpression *exp, int debugId, QQmlWatcher *parent)
    : QObject(parent),
      m_id(id),
      m_watch(parent),
      m_object(nullptr),
      m_debugId(debugId),
      m_expr(exp)
{
    QObject::connect(m_expr, &QQmlExpression::valueChanged,
                     this, &QQmlWatchProxy::notifyValueChanged);
}

QQmlWatchProxy::QQmlWatchProxy(int id, QObject *object, int debugId, const QMetaProperty &prop,
                               QQmlWatcher *parent)
    : QObject(parent),
      m_id(id),
      m_watch(parent),
      m_object(object),
      m_debugId(debugId),
      m_property(prop),
      m_expr(nullptr)
{
    static int refreshIdx = -1;
    if(refreshIdx == -1)
        refreshIdx = QQmlWatchProxy::staticMetaObject.indexOfMethod("notifyValueChanged()");

    if (prop.hasNotifySignal())
        QQmlPropertyPrivate::connect(m_object, prop.notifySignalIndex(), this, refreshIdx);
}

void QQmlWatchProxy::notifyValueChanged()
{
    QVariant v;
    if (m_expr)
        v = m_expr->evaluate();
    else
        v = m_property.read(m_object);
    emit m_watch->propertyChanged(m_id, m_debugId, m_property, v);
}


QQmlWatcher::QQmlWatcher(QObject *parent)
    : QObject(parent)
{
}

bool QQmlWatcher::addWatch(int id, quint32 debugId)
{
    QObject *object = QQmlDebugService::objectForId(debugId);
    if (object) {
        int propCount = object->metaObject()->propertyCount();
        for (int ii=0; ii<propCount; ii++)
            addPropertyWatch(id, object, debugId, object->metaObject()->property(ii));
        return true;
    }
    return false;
}

bool QQmlWatcher::addWatch(int id, quint32 debugId, const QByteArray &property)
{
    QObject *object = QQmlDebugService::objectForId(debugId);
    if (object) {
        int index = object->metaObject()->indexOfProperty(property.constData());
        if (index >= 0) {
            addPropertyWatch(id, object, debugId, object->metaObject()->property(index));
            return true;
        }
    }
    return false;
}

bool QQmlWatcher::addWatch(int id, quint32 objectId, const QString &expr)
{
    QObject *object = QQmlDebugService::objectForId(objectId);
    QQmlContext *context = qmlContext(object);
    if (context) {
        QQmlExpression *exprObj = new QQmlExpression(context, object, expr);
        exprObj->setNotifyOnValueChanged(true);
        QQmlWatchProxy *proxy = new QQmlWatchProxy(id, exprObj, objectId, this);
        exprObj->setParent(proxy);
        m_proxies[id].append(proxy);
        proxy->notifyValueChanged();
        return true;
    }
    return false;
}

bool QQmlWatcher::removeWatch(int id)
{
    if (!m_proxies.contains(id))
        return false;

    QList<QPointer<QQmlWatchProxy> > proxies = m_proxies.take(id);
    qDeleteAll(proxies);
    return true;
}

void QQmlWatcher::addPropertyWatch(int id, QObject *object, quint32 debugId, const QMetaProperty &property)
{
    QQmlWatchProxy *proxy = new QQmlWatchProxy(id, object, debugId, property, this);
    m_proxies[id].append(proxy);

    proxy->notifyValueChanged();
}

QT_END_NAMESPACE

#include <qqmlwatcher.moc>

#include "moc_qqmlwatcher.cpp"
