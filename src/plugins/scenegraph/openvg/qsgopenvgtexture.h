// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    qint64 comparisonKey() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;

private:
    VGImage m_image;
};

QT_END_NAMESPACE

#endif // QSGOPENVGTEXTURE_H
