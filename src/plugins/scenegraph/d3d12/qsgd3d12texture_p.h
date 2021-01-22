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

#ifndef QSGD3D12TEXTURE_P_H
#define QSGD3D12TEXTURE_P_H

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

#include <private/qsgtexture_p.h>
#include <basetsd.h>

QT_BEGIN_NAMESPACE

class QSGD3D12Engine;
class QSGD3D12TexturePrivate;

class QSGD3D12Texture : public QSGTexture
{
    Q_DECLARE_PRIVATE(QSGD3D12Texture)
public:
    QSGD3D12Texture(QSGD3D12Engine *engine);
    ~QSGD3D12Texture();

    void create(const QImage &image, uint flags);

    int textureId() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    QRectF normalizedTextureSubRect() const override;
    void bind() override;

protected:
    QSGD3D12Engine *m_engine;
    QImage m_image;
    bool m_createPending = false;
    bool m_createdWithMipMaps = false;
    uint m_id = 0;
    bool m_alphaWanted = false;
};

class QSGD3D12TexturePrivate : public QSGTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGD3D12Texture)
public:
    int comparisonKey() const override;
};

QT_END_NAMESPACE

#endif
