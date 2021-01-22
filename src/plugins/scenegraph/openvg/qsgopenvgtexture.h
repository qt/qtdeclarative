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

#ifndef QSGOPENVGTEXTURE_H
#define QSGOPENVGTEXTURE_H

#include <private/qsgtexture_p.h>

#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

class QSGOpenVGTexture : public QSGTexture
{
public:
    QSGOpenVGTexture(const QImage &image, uint flags);
    ~QSGOpenVGTexture();

    int textureId() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    void bind() override;

private:
    VGImage m_image;;
};

QT_END_NAMESPACE

#endif // QSGOPENVGTEXTURE_H
