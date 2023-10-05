// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPEABSTRACTCURVENODE_P_H
#define QQUICKSHAPEABSTRACTCURVENODE_P_H

#include <QtQuick/qsgnode.h>

#include "qquickshapegenericrenderer_p.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQuickShapeAbstractCurveNode : public QSGGeometryNode
{
public:
    virtual void setColor(QColor col) = 0;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPEABSTRACTCURVENODE_P_H
