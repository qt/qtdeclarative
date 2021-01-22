/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqmllistaccessor_p.h"

#include <private/qqmlmetatype_p.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>

// ### Remove me
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QQmlListAccessor::QQmlListAccessor()
: m_type(Invalid)
{
}

QQmlListAccessor::~QQmlListAccessor()
{
}

QVariant QQmlListAccessor::list() const
{
    return d;
}

void QQmlListAccessor::setList(const QVariant &v, QQmlEngine *engine)
{
    d = v;

    // An incoming JS array as model is treated as a variant list, so we need to
    // convert it first with toVariant().
    if (d.userType() == qMetaTypeId<QJSValue>())
        d = d.value<QJSValue>().toVariant();

    QQmlEnginePrivate *enginePrivate = engine?QQmlEnginePrivate::get(engine):nullptr;

    if (!d.isValid()) {
        m_type = Invalid;
    } else if (d.userType() == QMetaType::QStringList) {
        m_type = StringList;
    } else if (d.userType() == QMetaType::QVariantList) {
        m_type = VariantList;
    } else if (d.userType() == qMetaTypeId<QList<QObject *>>()) {
        m_type = ObjectList;
    } else if (d.canConvert(QMetaType::Int)) {
        // Here we have to check for an upper limit, because down the line code might (well, will)
        // allocate memory depending on the number of elements. The upper limit cannot be INT_MAX:
        //      QVector<QPointer<QQuickItem>> something;
        //      something.resize(count());
        // (See e.g. QQuickRepeater::regenerate())
        // This will allocate data along the lines of:
        //      sizeof(QPointer<QQuickItem>) * count() + QVector::headerSize
        // So, doing an approximate round-down-to-nice-number, we get:
        const int upperLimit = 100 * 1000 * 1000;

        int i = v.toInt();
        if (i < 0) {
            qWarning("Model size of %d is less than 0", i);
            m_type = Invalid;
        } else if (i > upperLimit) {
            qWarning("Model size of %d is bigger than the upper limit %d", i, upperLimit);
            m_type = Invalid;
        } else {
            m_type = Integer;
        }
    } else if ((!enginePrivate && QQmlMetaType::isQObject(d.userType())) ||
               (enginePrivate && enginePrivate->isQObject(d.userType()))) {
        QObject *data = enginePrivate?enginePrivate->toQObject(d):QQmlMetaType::toQObject(d);
        d = QVariant::fromValue(data);
        m_type = Instance;
    } else if (d.userType() == qMetaTypeId<QQmlListReference>()) {
        m_type = ListProperty;
    } else {
        m_type = Instance;
    }
}

int QQmlListAccessor::count() const
{
    switch(m_type) {
    case StringList:
        return qvariant_cast<QStringList>(d).count();
    case VariantList:
        return qvariant_cast<QVariantList>(d).count();
    case ObjectList:
        return qvariant_cast<QList<QObject *>>(d).count();
    case ListProperty:
        return ((const QQmlListReference *)d.constData())->count();
    case Instance:
        return 1;
    case Integer:
        return d.toInt();
    default:
    case Invalid:
        return 0;
    }
}

QVariant QQmlListAccessor::at(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < count());
    switch(m_type) {
    case StringList:
        return QVariant::fromValue(qvariant_cast<QStringList>(d).at(idx));
    case VariantList:
        return qvariant_cast<QVariantList>(d).at(idx);
    case ObjectList:
        return QVariant::fromValue(qvariant_cast<QList<QObject *>>(d).at(idx));
    case ListProperty:
        return QVariant::fromValue(((const QQmlListReference *)d.constData())->at(idx));
    case Instance:
        return d;
    case Integer:
        return QVariant(idx);
    default:
    case Invalid:
        return QVariant();
    }
}

bool QQmlListAccessor::isValid() const
{
    return m_type != Invalid;
}

QT_END_NAMESPACE
