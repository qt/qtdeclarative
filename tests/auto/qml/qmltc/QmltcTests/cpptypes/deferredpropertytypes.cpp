// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deferredpropertytypes.h"

QQuickItem *TypeWithDeferredProperty::deferredProperty() const
{
    return m_deferredProperty;
}

void TypeWithDeferredProperty::setDeferredProperty(QQuickItem *value)
{
    if (m_deferredProperty != value)
        m_deferredProperty = value;
}

QBindable<QQuickItem *> TypeWithDeferredProperty::bindableDeferredProperty()
{
    return QBindable<QQuickItem *>(&m_deferredProperty);
}

TestTypeAttachedWithDeferred *DeferredAttached::qmlAttachedProperties(QObject *parent)
{
    return new TestTypeAttachedWithDeferred(parent);
}
