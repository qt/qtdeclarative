// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef OVERRIDDENATTACHEDSLOT_H
#define OVERRIDDENATTACHEDSLOT_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqml.h>

// The base class has a slot before processData() so that the relative method indices of
// processData() differ between base and derived class.
class OverriddenSlotReceiver : public QObject
{
    Q_OBJECT
public:
    OverriddenSlotReceiver(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    virtual void precedingSlot() {}
    virtual int processData(const QVariantMap &data = QVariantMap()) { Q_UNUSED(data); return -1; }
};

class OverriddenSlotDerived : public OverriddenSlotReceiver
{
    Q_OBJECT
public:
    OverriddenSlotDerived(QObject *parent = nullptr) : OverriddenSlotReceiver(parent) {}

public slots:
    int processData(const QVariantMap &data = QVariantMap()) override { return data.size(); }
};

class OverriddenSlotHolder : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(OverriddenSlotReceiver)
public:
    OverriddenSlotHolder(QObject *parent = nullptr) : QObject(parent) {}

    static OverriddenSlotReceiver *qmlAttachedProperties(QObject *object)
    {
        return new OverriddenSlotDerived(object);
    }
};

#endif // OVERRIDDENATTACHEDSLOT_H
