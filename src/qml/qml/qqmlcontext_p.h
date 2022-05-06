/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QQMLCONTEXT_P_H
#define QQMLCONTEXT_P_H

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

#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtCore/qpointer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmllist.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qtaggedpointer.h>
#include <QtQml/private/qqmlrefcount_p.h>

QT_BEGIN_NAMESPACE

class QQmlContextData;

class QQmlContextPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlContext)
public:
    static QQmlContextPrivate *get(QQmlContext *context) {
        return static_cast<QQmlContextPrivate *>(QObjectPrivate::get(context));
    }

    static QQmlContext *get(QQmlContextPrivate *context) {
        return static_cast<QQmlContext *>(context->q_func());
    }

    static qsizetype context_count(QQmlListProperty<QObject> *);
    static QObject *context_at(QQmlListProperty<QObject> *, qsizetype);

    void dropDestroyedQObject(const QString &name, QObject *destroyed);

    int notifyIndex() const { return m_notifyIndex; }
    void setNotifyIndex(int notifyIndex) { m_notifyIndex = notifyIndex; }

    int numPropertyValues() const { return m_propertyValues.count(); }
    void appendPropertyValue(const QVariant &value) { m_propertyValues.append(value); }
    void setPropertyValue(int index, const QVariant &value) { m_propertyValues[index] = value; }
    QVariant propertyValue(int index) const { return m_propertyValues[index]; }

    QList<QPointer<QObject>> instances() const { return m_instances; }
    void appendInstance(QObject *instance) { m_instances.append(instance); }
    void cleanInstances()
    {
        for (auto it = m_instances.begin(); it != m_instances.end();
             it->isNull() ? (it = m_instances.erase(it)) : ++it) {}
    }

    void emitDestruction();

private:
    friend class QQmlContextData;

    QQmlContextPrivate(QQmlContextData *data) : m_data(data) {}
    QQmlContextPrivate(QQmlContext *publicContext, QQmlContextData *parent,
                       QQmlEngine *engine = nullptr);

    // Intentionally a bare pointer. QQmlContextData knows whether it owns QQmlContext or vice
    // versa. If QQmlContext is the owner, QQmlContextData keeps an extra ref for its publicContext.
    // The publicContext ref is released when doing QQmlContextData::setPublicContext(nullptr).
    QQmlContextData *m_data;

    QList<QVariant> m_propertyValues;
    int m_notifyIndex = -1;

    // Only used for debugging
    QList<QPointer<QObject>> m_instances;
};

QT_END_NAMESPACE

#endif // QQMLCONTEXT_P_H
