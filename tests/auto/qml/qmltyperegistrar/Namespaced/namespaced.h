// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef NAMESPACED_H
#define NAMESPACED_H

#include "tst_qmltyperegistrar_namespaced_export.h"

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

class Namespaced : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Namespaced(QObject *parent = nullptr);
};

#endif // NAMESPACED_H
