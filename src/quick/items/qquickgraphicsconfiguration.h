/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
