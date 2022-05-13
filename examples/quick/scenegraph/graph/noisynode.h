// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef NOISYNODE_H
#define NOISYNODE_H

#include <QSGGeometryNode>
#include <QQuickWindow>

class NoisyNode : public QSGGeometryNode
{
public:
    NoisyNode(QQuickWindow *window);

    void setRect(const QRectF &bounds);
};

#endif // NOISYNODE_H
