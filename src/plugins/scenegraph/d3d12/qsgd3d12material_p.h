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

#ifndef QSGD3D12MATERIAL_P_H
#define QSGD3D12MATERIAL_P_H

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

#include <QtQuick/qsgmaterial.h>
#include "qsgd3d12engine_p.h"

QT_BEGIN_NAMESPACE

class QSGRenderer;

// The D3D renderer works with QSGD3D12Material as the "base" class since
// QSGMaterial and its GL program related bits are not suitable. Also, there is
// no split like with QSGMaterialShader.

typedef QSGMaterialShader::RenderState QSGD3D12MaterialRenderState;

class QSGD3D12Material : public QSGMaterial
{
public:
    struct ExtraState {
        QVector4D blendFactor;
    };

    enum UpdateResult {
        UpdatedShaders = 0x0001,
        UpdatedConstantBuffer = 0x0002,
        UpdatedBlendFactor = 0x0004
    };
    Q_DECLARE_FLAGS(UpdateResults, UpdateResult)

    virtual int constantBufferSize() const = 0;
    virtual void preparePipeline(QSGD3D12PipelineState *pipelineState) = 0;
    virtual UpdateResults updatePipeline(const QSGD3D12MaterialRenderState &state,
                                         QSGD3D12PipelineState *pipelineState,
                                         ExtraState *extraState,
                                         quint8 *constantBuffer) = 0;

private:
    QSGMaterialShader *createShader() const override; // dummy, QSGMaterialShader is too GL dependent
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGD3D12Material::UpdateResults)

QT_END_NAMESPACE

#endif // QSGD3D12MATERIAL_P_H
