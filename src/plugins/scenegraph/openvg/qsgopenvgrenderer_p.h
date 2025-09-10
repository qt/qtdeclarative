// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGRENDERER_H
#define QSGOPENVGRENDERER_H

#include <private/qsgrenderer_p.h>

QT_BEGIN_NAMESPACE

class QSGOpenVGRenderer : public QSGRenderer
{
public:
    QSGOpenVGRenderer(QSGRenderContext *context);
    virtual ~QSGOpenVGRenderer();

    void nodeChanged(QSGNode *node, QSGNode::DirtyState state) override;

    void render() final;
};

QT_END_NAMESPACE

#endif // QSGOPENVGRENDERER_H
