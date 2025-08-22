// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLECONSTANTS_H
#define QQUICKSTYLECONSTANTS_H

#include <QtQml/QtQml>

QT_BEGIN_NAMESPACE

class QQuickStyleConstants : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool runningWithLiquidGlass READ runningWithLiquidGlass CONSTANT FINAL)
    Q_PROPERTY(QColor secondarySystemFillColor READ secondarySystemFillColor CONSTANT FINAL)
    Q_PROPERTY(QColor tertiarySystemFillColor READ tertiarySystemFillColor CONSTANT FINAL)
    QML_NAMED_ELEMENT(StyleConstants)
    QML_SINGLETON

public:
    QQuickStyleConstants();

    bool runningWithLiquidGlass() const;
    QColor secondarySystemFillColor() const;
    QColor tertiarySystemFillColor() const;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLECONSTANTS_H
