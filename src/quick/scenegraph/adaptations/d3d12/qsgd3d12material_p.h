/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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

class QSGD3D12Material : public QSGMaterial
{
public:
    struct RenderState {
        enum DirtyState {
            DirtyMatrix = 0x0001,
            DirtyOpacity = 0x0002,
            DirtyAll = 0xFFFF
        };
        Q_DECLARE_FLAGS(DirtyStates, DirtyState)

        DirtyStates dirtyStates() const { return m_dirty; }

        bool isMatrixDirty() const { return m_dirty & DirtyMatrix; }
        bool isOpacityDirty() const { return m_dirty & DirtyOpacity; }

        float opacity() const;
        QMatrix4x4 combinedMatrix() const;
        QMatrix4x4 modelViewMatrix() const;
        QMatrix4x4 projectionMatrix() const;
        QRect viewportRect() const;
        QRect deviceRect() const;
        float determinant() const;
        float devicePixelRatio() const;

        DirtyStates m_dirty = 0;
        void *m_data = nullptr;
    };

    enum UpdateResult {
        UpdatedShaders = 0x0001,
        UpdatedConstantBuffer = 0x0002
    };
    Q_DECLARE_FLAGS(UpdateResults, UpdateResult)

    static RenderState makeRenderState(QSGRenderer *renderer, RenderState::DirtyStates dirty);

    virtual int constantBufferSize() const = 0;
    virtual void preparePipeline(QSGD3D12ShaderState *shaders) = 0;
    virtual UpdateResults updatePipeline(const RenderState &state,
                                         QSGD3D12ShaderState *shaders,
                                         quint8 *constantBuffer) = 0;

private:
    QSGMaterialShader *createShader() const override; // dummy, QSGMaterialShader is too GL dependent
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGD3D12Material::RenderState::DirtyStates)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSGD3D12Material::UpdateResults)

class QSGD3D12VertexColorMaterial : public QSGD3D12Material
{
public:
    QSGMaterialType *type() const override;
    int compare(const QSGMaterial *other) const override;

    virtual int constantBufferSize() const override;
    void preparePipeline(QSGD3D12ShaderState *shaders) override;
    UpdateResults updatePipeline(const RenderState &state,
                                 QSGD3D12ShaderState *shaders,
                                 quint8 *constantBuffer) override;

private:
    static QSGMaterialType mtype;
};

QT_END_NAMESPACE

#endif // QSGD3D12MATERIAL_P_H
