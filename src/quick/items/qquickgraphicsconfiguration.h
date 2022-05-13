// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKGRAPHICSCONFIGURATION_H
#define QQUICKGRAPHICSCONFIGURATION_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/QByteArrayList>

QT_BEGIN_NAMESPACE

class QQuickGraphicsConfigurationPrivate;

class Q_QUICK_EXPORT QQuickGraphicsConfiguration
{
public:
    QQuickGraphicsConfiguration();
    ~QQuickGraphicsConfiguration();
    QQuickGraphicsConfiguration(const QQuickGraphicsConfiguration &other);
    QQuickGraphicsConfiguration &operator=(const QQuickGraphicsConfiguration &other);

    static QByteArrayList preferredInstanceExtensions();

    void setDeviceExtensions(const QByteArrayList &extensions);
    QByteArrayList deviceExtensions() const;

    void setDepthBufferFor2D(bool enable);
    bool isDepthBufferEnabledFor2D() const;

private:
    void detach();
    QQuickGraphicsConfigurationPrivate *d;
    friend class QQuickGraphicsConfigurationPrivate;
};

QT_END_NAMESPACE

#endif // QQUICKGRAPHICSCONFIGURATION_H
