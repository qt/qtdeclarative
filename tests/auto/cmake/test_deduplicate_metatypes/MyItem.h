// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MYITEM_H
#define MYITEM_H

#include <QtQml/qqmlcomponent.h>
#include <QtQmlIntegration/qqmlintegration.h>

class MyItem : public QQmlComponent
{
    Q_OBJECT
    QML_ELEMENT

public:
    using QQmlComponent::QQmlComponent;
};

#endif
