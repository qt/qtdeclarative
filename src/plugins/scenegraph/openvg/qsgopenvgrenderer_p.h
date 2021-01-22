/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

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

    void renderScene(uint fboId = 0) final;
    void render() final;
};

QT_END_NAMESPACE

#endif // QSGOPENVGRENDERER_H
