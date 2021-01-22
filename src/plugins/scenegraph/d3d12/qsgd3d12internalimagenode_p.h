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

#ifndef QSGD3D12INTERNALIMAGENODE_P_H
#define QSGD3D12INTERNALIMAGENODE_P_H

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

#include <private/qsgbasicinternalimagenode_p.h>
#include "qsgd3d12builtinmaterials_p.h"

QT_BEGIN_NAMESPACE

class QSGD3D12InternalImageNode : public QSGBasicInternalImageNode
{
public:
    QSGD3D12InternalImageNode();

    void setMipmapFiltering(QSGTexture::Filtering filtering) override;
    void setFiltering(QSGTexture::Filtering filtering) override;
    void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode) override;
    void setVerticalWrapMode(QSGTexture::WrapMode wrapMode) override;

    void updateMaterialAntialiasing() override;
    void setMaterialTexture(QSGTexture *texture) override;
    QSGTexture *materialTexture() const override;
    bool updateMaterialBlending() override;
    bool supportsWrap(const QSize &size) const override;

private:
    QSGD3D12TextureMaterial m_material;
    QSGD3D12SmoothTextureMaterial m_smoothMaterial;
};

QT_END_NAMESPACE

#endif // QSGD3D12INTERNALIMAGENODE_P_H
