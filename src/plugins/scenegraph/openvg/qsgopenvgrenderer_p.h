// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
