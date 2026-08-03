// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIOBJECTREGISTRYREF_H
#define QMULTIOBJECTREGISTRYREF_H

#include <QtQmlDesignSupport/qabstractobjectregistryref.h>

QT_BEGIN_NAMESPACE

class QMultiObjectRegistryRefPrivate;
class QQmlEngine;

class Q_QMLDESIGNSUPPORT_EXPORT QMultiObjectRegistryRef : public QAbstractObjectRegistryRef
{
    Q_OBJECT

    Q_DECLARE_PRIVATE(QMultiObjectRegistryRef)

    Q_PROPERTY(QQmlListProperty<QObject> objects READ objects NOTIFY objectsChanged FINAL)

    QML_NAMED_ELEMENT(MultiObjectRegistryRef)

public:
    explicit QMultiObjectRegistryRef(QObject *parent = nullptr);
    explicit QMultiObjectRegistryRef(QQmlEngine *engine, QObject *parent = nullptr);
    explicit QMultiObjectRegistryRef(QQmlEngine *engine, const QString &key,
                                     QObject *parent = nullptr);
    ~QMultiObjectRegistryRef() override;

    QQmlListProperty<QObject> objects();
    QList<QObject *> objectsList();

Q_SIGNALS:
    void objectsChanged();
    void objectAdded(QObject *obj);
    void objectRemoved(QObject *obj);
};

QT_END_NAMESPACE

#endif // QMULTIOBJECTREGISTRYREF_H
