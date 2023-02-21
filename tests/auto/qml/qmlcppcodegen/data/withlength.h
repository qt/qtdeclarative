// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WITHLENGTH_H
#define WITHLENGTH_H

#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtQmlIntegration/qqmlintegration.h>

struct ValueTypeWithLength
{
    Q_GADGET
    QML_VALUE_TYPE(withLength)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(int length READ length CONSTANT)

public:
    ValueTypeWithLength() = default;
    Q_INVOKABLE ValueTypeWithLength(int length) : m_length(length) {}
    Q_INVOKABLE ValueTypeWithLength(QPointF point) : m_length(point.manhattanLength()) {}
    Q_INVOKABLE ValueTypeWithLength(QRectF rect) : m_length(rect.width()) {}
    Q_INVOKABLE QString toString() const { return QStringLiteral("no"); }

    int length() const { return m_length; }

private:
    int m_length = 19;
};

#endif // WITHLENGTH_H
