// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qquickdragaxis_p.h"
#include "qquickpointerhandler_p.h"
#include <QtQuick/qquickitem.h>
#include <limits>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcDragAxis, "qt.quick.pointer.dragaxis")

QQuickDragAxis::QQuickDragAxis(QQuickPointerHandler *handler, const QString &propertyName, qreal initValue)
  : QObject(handler), m_accumulatedValue(initValue), m_propertyName(propertyName)
{
}

void QQuickDragAxis::setMinimum(qreal minimum)
{
    if (m_minimum == minimum)
        return;

    m_minimum = minimum;
    emit minimumChanged();
}

void QQuickDragAxis::setMaximum(qreal maximum)
{
    if (m_maximum == maximum)
        return;

    m_maximum = maximum;
    emit maximumChanged();
}

void QQuickDragAxis::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged();
}

void QQuickDragAxis::onActiveChanged(bool active, qreal initActiveValue)
{
    m_activeValue = initActiveValue;
    m_startValue = m_accumulatedValue;
    qCDebug(lcDragAxis) << parent() << m_propertyName << active << ": init active" << m_activeValue
                        << "target start" << m_startValue;
}

void QQuickDragAxis::updateValue(qreal activeValue, qreal accumulatedValue, qreal delta)
{
    if (!m_enabled)
        return;

    m_activeValue = activeValue;
    m_accumulatedValue = qBound(m_minimum, accumulatedValue, m_maximum);
    qCDebug(lcDragAxis) << parent() << m_propertyName << "values: active" << activeValue
                        << "accumulated" << m_accumulatedValue << "delta" << delta;
    emit activeValueChanged(delta);
}

QT_END_NAMESPACE

#include "moc_qquickdragaxis_p.cpp"
