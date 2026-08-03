// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOBJECTREGISTRYREF_H
#define QOBJECTREGISTRYREF_H

#include <QtQmlDesignSupport/qabstractobjectregistryref.h>

QT_BEGIN_NAMESPACE

class QObjectRegistryRefPrivate;
class QQmlEngine;

class Q_QMLDESIGNSUPPORT_EXPORT QObjectRegistryRef : public QAbstractObjectRegistryRef
{
    Q_OBJECT

    Q_DECLARE_PRIVATE(QObjectRegistryRef)

    Q_PROPERTY(QObject *object READ object NOTIFY objectChanged FINAL)

    QML_NAMED_ELEMENT(ObjectRegistryRef)

public:
    explicit QObjectRegistryRef(QObject *parent = nullptr);
    explicit QObjectRegistryRef(QQmlEngine *engine, QObject *parent = nullptr);
    explicit QObjectRegistryRef(QQmlEngine *engine, const QString &key, QObject *parent = nullptr);
    ~QObjectRegistryRef() override;

    QObject *object() const;

Q_SIGNALS:
    void objectChanged();
};

QT_END_NAMESPACE

#endif // QOBJECTREGISTRYREF_H
