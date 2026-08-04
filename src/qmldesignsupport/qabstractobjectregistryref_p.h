// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QABSTRACTOBJECTREGISTRYREF_P_H
#define QABSTRACTOBJECTREGISTRYREF_P_H

#include <QtQmlDesignSupport/qabstractobjectregistryref.h>

#include <QtQml/qqml.h>

#include <QtCore/qpointer.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QObjectRegistrySingleton;
class QQmlEngine;

class Q_QMLDESIGNSUPPORT_EXPORT QAbstractObjectRegistryRefPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractObjectRegistryRef)

public:
    explicit QAbstractObjectRegistryRefPrivate(QQmlEngine *engine = nullptr);

    virtual void handleObjectAdded(QObject *obj) = 0;
    virtual void handleObjectRemoved(QObject *obj) = 0;

    QString key() const;

protected:
    virtual void handleInitialObjects() = 0;

    QObjectRegistrySingleton *registry() const;

private:
    static void dataAppend(QQmlListProperty<QObject> *l, QObject *o);
    static qsizetype dataCount(QQmlListProperty<QObject> *l);
    static QObject *dataAt(QQmlListProperty<QObject> *l, qsizetype i);
    static void dataClear(QQmlListProperty<QObject> *l);
    static void dataReplace(QQmlListProperty<QObject> *l, qsizetype i, QObject *o);
    static void dataRemoveLast(QQmlListProperty<QObject> *l);

    QString m_key;
    QList<QObject *> m_data;
    QPointer<QObjectRegistrySingleton> m_registry;
};

QT_END_NAMESPACE

#endif // QABSTRACTOBJECTREGISTRYREF_P_H
