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

#ifndef QSGD3D12INTERNALRECTANGLENODE_P_H
#define QSGD3D12INTERNALRECTANGLENODE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qsgbasicinternalrectanglenode_p.h>
#include "qsgd3d12builtinmaterials_p.h"

QT_BEGIN_NAMESPACE

class QSGD3D12InternalRectangleNode : public QSGBasicInternalRectangleNode
{
public:
    QSGD3D12InternalRectangleNode();

private:
    void updateMaterialAntialiasing() override;
    void updateMaterialBlending(QSGNode::DirtyState *state) override;

    QSGD3D12VertexColorMaterial m_material;
    QSGD3D12SmoothColorMaterial m_smoothMaterial;
};

QT_END_NAMESPACE

#endif // QSGD3D12INTERNALRECTANGLENODE_P_H
